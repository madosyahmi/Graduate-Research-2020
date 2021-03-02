// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <SNS_BME280>
#include <SNS_SHT3X>
#include <STG_STD>

/// if use with PAL board, define this.
#undef USE_PAL 
#ifdef USE_PAL
// X_STR makes string literals.
#define X_STR(s) TO_STR(s)
#define TO_STR(s) #s
#include X_STR(USE_PAL) // just use with PAL board (to handle WDT)
#endif

#define USE_RECIEVE //自分で作成、受信待機をする
#define DEVID0f //論理IDを0x0fで指定
#define CO2_WAIT 90000 //CO2センサのためにヒートアップする待機時間を指定
#define CO2_SAMPLING_DUR 10000 //CO2センサのサンプリング間隔。3回計測します。すなわち計測のときは2回delay実行されます

/*** Config part */
// application ID
const uint32_t DEF_APPID = 0x1234abcd;
uint32_t APP_ID = DEF_APPID;

// channel
uint8_t CHANNEL = 13;

// id
uint8_t u8ID = 0;

// application use
const char_t APP_NAME[] = "ENV SENSOR";
const uint8_t FOURCHARS[] = "SBS1";

uint8_t u8txid = 0;
uint32_t u32tick_tx;

uint32_t last_chg_LED;

// very simple state machine
enum class E_STATE {
    INIT = 0,
    CAPTURE_PRE,
    CAPTURE,
    TX,
    TX_WAIT_COMP,
	SETTING_MODE,
    SUCCESS,
    ERROR,
	RECIEVE_WAIT,
	LONG_SLEEP,
	CO2_PREHEAT
};
#ifdef USE_RECIEVE
E_STATE eState = E_STATE::RECIEVE_WAIT;
#else
E_STATE eState = E_STATE::INIT;
#endif

#ifdef USE_RECIEVE
//受信に関するいろいろ
uint8_t rx_cmd; //コマンド種別(1バイト、とりあえず00)
uint16_t during_sec; //送信間隔秒(2バイト)
uint16_t send_no; //送信回数(2バイト)
uint32_t wait_sec; //休眠秒(4バイト)
#else
uint8_t rx_cmd = 0x00; //コマンド種別(1バイト、とりあえず00)
uint16_t during_sec = 180; //送信間隔秒(2バイト)
uint16_t send_no = 240; //送信回数(2バイト)
uint32_t wait_sec = 43200; //休眠秒(4バイト)
#endif

/*** Local function prototypes */
void sleepNow();

/*** sensor objects */
SNS_BME280 sns_bme280;
char_t bme280_model[8] = "BME280";
bool b_found_bme280 = false;
bool b_bme280_w_humid = false;

SNS_SHT3X sns_sht3x;
bool b_found_sht3x = false;

uint16_t u16_volt_vcc = 0;
uint16_t u16_volt_a1 = 0;

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */
#ifdef USE_PAL
	/// use PAL board (for WDT handling)
    auto&& brd = the_twelite.board.use<USE_PAL>(); // register board (PAL)
#endif

	/// interactive mode settings
	auto&& set = the_twelite.settings.use<STG_STD>();
			// declare to use interactive setting.
			// once activated, use `set.serial' instead of `Serial'.

	set << SETTINGS::appname(APP_NAME)          // set application name appears in interactive setting menu.
		<< SETTINGS::appid_default(DEF_APPID); // set the default application ID.

	set.hide_items(
		  E_STGSTD_SETID::POWER_N_RETRY
		, E_STGSTD_SETID::OPT_DWORD1
		, E_STGSTD_SETID::OPT_DWORD2
		, E_STGSTD_SETID::OPT_DWORD3
		, E_STGSTD_SETID::OPT_DWORD4
		, E_STGSTD_SETID::ENC_MODE
		, E_STGSTD_SETID::ENC_KEY_STRING
	);

	// read SET pin (DIO12)
	pinMode(PIN_DIGITAL::DIO12, PIN_MODE::INPUT_PULLUP);
	if (digitalRead(PIN_DIGITAL::DIO12) == LOW) {
		set << SETTINGS::open_at_start();      // start interactive mode immediately.
		eState = E_STATE::SETTING_MODE;        // set state in loop() as dedicated mode for settings.
		return;                                // skip standard initialization.
	}
	//DIO19ピンは昇圧センサEN端子に接続。二酸化炭素センサの電源供給をコントロール
	pinMode(PIN_DIGITAL::DIO19, PIN_MODE::OUTPUT);
	digitalWrite(PIN_DIGITAL::DIO19,HIGH);
	//DIO18ピンはLEDに接続。
	pinMode(PIN_DIGITAL::DIO18, PIN_MODE::OUTPUT);
	digitalWrite(PIN_DIGITAL::DIO18,LOW);

	// acquired EEPROM saved data	
	set.reload(); // must call this before getting data, if configuring method is called.

	APP_ID = set.u32appid();
	CHANNEL = set.u8ch();
	#ifdef DEVID0f
	u8ID = 0x0f;
	#else
	u8ID = set.u8devid();
	#endif

	// the twelite main object.
	the_twelite
		<< TWENET::appid(APP_ID)     // set application ID (identify network group)
	#ifdef USE_RECIEVE
		<< TWENET::channel(CHANNEL) // set channel (pysical channel)
		<< TWENET::rx_when_idle();   // 受信も行う
	#else
		<< TWENET::channel(CHANNEL); // set channel (pysical channel)
	#endif

	// Register Network
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>();
	nwk << NWK_SIMPLE::logical_id(u8ID); // set Logical ID. (0xFE means a child device with no ID)

	/*** BEGIN section */
	/*** INIT message */
	Serial << crlf << "--- " << APP_NAME << ":" << FOURCHARS << " ---";

	Serial << crlf << format("..APP_ID   = %08X", APP_ID);
	Serial << crlf << format("..CHANNEL  = %d", CHANNEL);
	Serial << crlf << format("..LID      = %d(0x%02X)", u8ID, u8ID);

	// sensors.setup() may call Wire during initialization.
	Wire.begin(WIRE_CONF::WIRE_100KHZ);

	// setup analogue
	Analogue.setup();

	// check SHT3x
	{
		bool b_alt_id = false;
		sns_sht3x.setup();
		if (!sns_sht3x.probe()) {
			bool b_alt_id = false;
			delayMicroseconds(100); // just in case, wait for devices to listen furthre I2C comm.
			sns_sht3x.setup(0x45); // alternative ID
			if (sns_sht3x.probe()) b_found_sht3x = true;
		} else {
			b_found_sht3x = true;
		}
		if (b_found_sht3x) {
			Serial << crlf << "..found sht3x" << (b_alt_id ? " at 0x45" : " at 0x44");
		}
	}


	delayMicroseconds(100); // just in case, wait for devices to listen furthre I2C comm.
	
	// check BMx280
	{
		bool b_alt_id = false;
		sns_bme280.setup();
		if (!sns_bme280.probe()) {
			b_alt_id = true;
			delayMicroseconds(100); // just in case, wait for devices to listen furthre I2C comm.
			sns_bme280.setup(0x77); // alternative ID
			if (sns_bme280.probe()) b_found_bme280 = true;
		} else {
			b_found_bme280 = true;
		}

		if (b_found_bme280) {
			// check if BME280 or BMP280	
			if ((sns_bme280.sns_stat() & 0xFF) == 0x60) {
				b_bme280_w_humid = true;
			} else
			if ((sns_bme280.sns_stat() & 0xFF) == 0x58) {
				b_bme280_w_humid = false;
				bme280_model[2] = 'P';
			}
			Serial << crlf
				<< format("..found %s ID=%02X", bme280_model, (sns_bme280.sns_stat() & 0xFF))
				<< (b_alt_id ? " at 0x77" : " at 0x76");
		}
	}

	//LEDの点滅処理のため、最後につけた時のtimeを記録しておく
	last_chg_LED = millis();

	/*** let the_twelite begin! */
	the_twelite.begin(); // start twelite!

}

/*** loop procedure (called every event) */
void loop() {
	bool new_state;

	//初回時Analog端子の取得
	if (Analogue.available()) {
		if (!u16_volt_vcc) {
			#ifndef USE_RECIEVE
				delay(CO2_WAIT * 6 / 5);
			#endif
			u16_volt_vcc = Analogue.read(PIN_ANALOGUE::VCC);
			u16_volt_a1 =  Analogue.read(PIN_ANALOGUE::A1);
		}
	}

	do {
		new_state = false;

		switch(eState) {			
		case E_STATE::SETTING_MODE: // while in setting (interactive mode)
			break;

#ifdef USE_RECIEVE
		case E_STATE::RECIEVE_WAIT: //受信モード利用のときのみ。
			rx_cmd = 0xff;
			//待機状態LEDを0.5秒おきに点滅
			if(millis() - last_chg_LED > 500 && millis() - last_chg_LED < 1000){
				digitalWrite(PIN_DIGITAL::DIO18,HIGH);
			}
			else{
				digitalWrite(PIN_DIGITAL::DIO18,LOW);
				if(millis() - last_chg_LED >= 1000){
					last_chg_LED = millis();
				}
			}
			
			//受信データがあった場合
			while (the_twelite.receiver.available()){
				auto&& rx = the_twelite.receiver.read();
				expand_bytes(rx.get_payload().begin(), rx.get_payload().end(),
					rx_cmd, //コマンド種別(1バイト、とりあえず00)
					during_sec, //送信間隔秒(2バイト)
					send_no, //送信回数(2バイト)
					wait_sec //休眠秒(4バイト)
					);
			}
			//受信データの確認。コマンド部が0x00のときに受信待機状態を抜ける
			if(rx_cmd == 0x00){
				Serial << crlf << format("<<recieve cmd>> during_sec is %d,send_no is %d, wait_sec is %d.",
				during_sec,send_no,wait_sec);
				eState = E_STATE::INIT;
				digitalWrite(PIN_DIGITAL::DIO18,LOW);
				if(millis() < (CO2_WAIT * 6 / 5)){
					// co2センサが温まるまでは待機
					delay((CO2_WAIT * 6 / 5) -millis());
				}
				//重複防止のため0~1秒待機
				delay(random(0,1000));
				new_state = true;
			}
			break;
#endif

		case E_STATE::INIT:
			Serial << crlf << format("..%04d/start sensor capture.", millis() & 8191);
			
			// start sensor capture
			Analogue.begin(pack_bits(PIN_ANALOGUE::A1, PIN_ANALOGUE::VCC)); // _start continuous adc capture.

			if (b_found_sht3x) {
				sns_sht3x.begin();
			}

			if (b_found_bme280) {
				sns_bme280.begin();
			}

			eState =  E_STATE::CAPTURE_PRE;
			break;

		case E_STATE::CAPTURE_PRE: // wait for sensor capture completion
			if (TickTimer.available()) {
				if (b_found_bme280 && !sns_bme280.available()) {
					sns_bme280.process_ev(E_EVENT_TICK_TIMER);
				}
				if (b_found_sht3x && !sns_sht3x.available()) {
					sns_sht3x.process_ev(E_EVENT_TICK_TIMER);
				}

				// both sensors are finished.
				if (	(!b_found_bme280 || (b_found_bme280 && sns_bme280.available()))
					&&	(!b_found_sht3x  || (b_found_sht3x && sns_sht3x.available()))
				) {
					new_state = true; // do next state immediately.
					eState =  E_STATE::CAPTURE;
				}
			}
		break;

		case E_STATE::CAPTURE: // display sensor results
			if (b_found_sht3x) {
					Serial 
						<< crlf << format("..%04d/finish sensor capture.", millis() & 8191)
						<< crlf << "  SHT3X    : T=" << sns_sht3x.get_temp() << 'C'
						<< " H=" << sns_sht3x.get_humid() << '%';
			}
			if (b_found_bme280) {
					Serial
						<< crlf << "  " << bme280_model << "   : T=" << sns_bme280.get_temp() << 'C'
						<< " P=" << int(sns_bme280.get_press()) << "hP";
				if (b_bme280_w_humid)
					Serial 
						<< " H=" << sns_bme280.get_humid() << '%';
			}
			if (1) {
				uint16_t sum_co2 = 0;
				int get_cnt = 0;
				//自分で追加
					if (Analogue.available()) {
						u16_volt_vcc = Analogue.read(PIN_ANALOGUE::VCC);
						u16_volt_a1 =  Analogue.read(PIN_ANALOGUE::A1);
						//読み取った電圧の値を二酸化炭素濃度に変換
						u16_volt_a1 = (u16_volt_a1 - 400) * 5000 / (double)(2000 - 400);
					}

					if(u16_volt_a1 >= 5000){
						delay(random(100,1000)); //おかしい値が出たときは最高1秒待機でもう一度計測しておく
						if(Analogue.available()){
							Serial << crlf << format("A1 read retry");
							u16_volt_a1 =  Analogue.read(PIN_ANALOGUE::A1);
							u16_volt_a1 = (u16_volt_a1 - 400) * 5000 / (double)(2000 - 400);
						}
					}if(u16_volt_a1 < 5000){
						sum_co2 += u16_volt_a1;
						get_cnt++;
					}
					Serial
						<< crlf << format("  ADC      : CO2=%04dppm", u16_volt_a1);
					delay(CO2_SAMPLING_DUR);

					if (Analogue.available()) {
						u16_volt_a1 =  Analogue.read(PIN_ANALOGUE::A1);
						//読み取った電圧の値を二酸化炭素濃度に変換
						u16_volt_a1 = (u16_volt_a1 - 400) * 5000 / (double)(2000 - 400);
					}

					//2回目計測
					if(u16_volt_a1 >= 5000){
						delay(random(100,1000)); //おかしい値が出たときは最高1秒待機でもう一度計測しておく
						if(Analogue.available()){
							Serial << crlf << format("A1 read retry");
							u16_volt_a1 =  Analogue.read(PIN_ANALOGUE::A1);
							u16_volt_a1 = (u16_volt_a1 - 400) * 5000 / (double)(2000 - 400);
						}
					}if(u16_volt_a1 < 5000){
						sum_co2 += u16_volt_a1;
						get_cnt++;
					}
					Serial
						<< crlf << format("  ADC      : CO2=%04dppm", u16_volt_a1);
					delay(CO2_SAMPLING_DUR);

					//3回目計測
					if(u16_volt_a1 >= 5000){
						delay(random(100,1000)); //おかしい値が出たときは最高1秒待機でもう一度計測しておく
						if(Analogue.available()){
							Serial << crlf << format("A1 read retry");
							u16_volt_a1 =  Analogue.read(PIN_ANALOGUE::A1);
							u16_volt_a1 = (u16_volt_a1 - 400) * 5000 / (double)(2000 - 400);
						}
					}if(u16_volt_a1 < 5000){
						sum_co2 += u16_volt_a1;
						get_cnt++;
					}
					Serial
						<< crlf << format("  ADC      : CO2=%04dppm", u16_volt_a1);
					
					//平均値で算出。正常と思われる値がなければ最後の値を表示
					if(get_cnt != 0){
						u16_volt_a1 = sum_co2 / get_cnt;
					}

					//ここまで
					Serial
						<< crlf << format("  ADC      : Vcc=%dmV AVE_CO2=%04dppm", u16_volt_vcc, u16_volt_a1);
			}
			
			//5V電源供給やめ
			digitalWrite(PIN_DIGITAL::DIO19,LOW);
			//電源が少ないときランプ点灯
			if(u16_volt_vcc <= 2700){
				digitalWrite(PIN_DIGITAL::DIO18,HIGH);
			}
			else{
				digitalWrite(PIN_DIGITAL::DIO18,LOW);
			}
			new_state = true; // do next state immediately.
			eState =  E_STATE::TX;
			break;

		case E_STATE::TX: // place TX packet requiest.
			eState = E_STATE::ERROR; // change this when success TX request...

			if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {
				// set tx packet behavior
				pkt << tx_addr(0x00)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
					<< tx_retry(0x1) // set retry (0x1 send two times in total)
					<< tx_packet_delay(0, 0, 2); // send packet w/ delay

				// prepare packet payload
				pack_bytes(pkt.get_payload() // set payload data objects.
					, make_pair(FOURCHARS, 4)  // just to see packet identification, you can design in any.
					, uint16_t(sns_sht3x.get_temp_cent()) // temp
					, uint16_t(sns_sht3x.get_humid_per_dmil())
					, uint16_t(sns_bme280.get_temp_cent()) // temp
					, uint16_t(sns_bme280.get_humid_per_dmil())
					, uint16_t(sns_bme280.get_press())
					, uint16_t(u16_volt_vcc)
					, uint16_t(u16_volt_a1)
				);

				// do transmit
				MWX_APIRET ret = pkt.transmit();
				Serial << crlf << format("..%04d/transmit request by id = %d.", millis() & 8191, ret.get_value());
				
				if (ret) {
					u8txid = ret.get_value() & 0xFF;
					u32tick_tx = millis();
					eState = E_STATE::TX_WAIT_COMP;
				} else {
					Serial << crlf << "!FATAL: TX REQUEST FAILS. reset the system." << crlf;
				}
			} else {
				Serial << crlf << "!FATAL: MWX TX OBJECT FAILS. reset the system." << crlf;
			}
			break;

		case E_STATE::TX_WAIT_COMP: // wait TX packet completion.
			if (the_twelite.tx_status.is_complete(u8txid)) {
				Serial << crlf << format("..%04d/transmit complete.", millis() & 8191);
		
				// success on TX
				eState = E_STATE::SUCCESS;
				new_state = true;
			} else if (millis() - u32tick_tx > 3000) {
				Serial << crlf << "!FATAL: MWX TX OBJECT FAILS. reset the system." << crlf;
				eState = E_STATE::ERROR;
				new_state = true;
			} 
			break;

		case E_STATE::ERROR: // FATAL ERROR
			Serial.flush();
			delay(100);
			the_twelite.reset_system();
			break;

		case E_STATE::SUCCESS: // NORMAL EXIT (go into sleeping...)
			sleepNow();
			break;

		case E_STATE::CO2_PREHEAT:
			//nothing do
			break;
		}
	} while(new_state);
}

// perform sleeping
void sleepNow() {
	//uint32_t u32ct = 1750 + random(0,500);

	uint32_t u32ct = 0;
	if(--send_no > 0){
		//通常スリープ
		u32ct = ((during_sec * 1000) - CO2_WAIT - 250) + random(0,500);
	}else{
		//夜間などのディープスリープ
		u32ct = ((wait_sec*1000) - 300000 - CO2_WAIT); //念のため5分前から起床
		eState = E_STATE::LONG_SLEEP;
	}
	Serial << crlf << format("..%04d/sleeping %dms.", millis() % 8191, u32ct);
	Serial.flush();

	if(eState != E_STATE::LONG_SLEEP){
		the_twelite.sleep(u32ct,true,false);
	}else{
		the_twelite.sleep(u32ct,true,true);
	}
}

//センサ値が安定するまで待機
void CO2_heat_wait(){
	Serial << crlf << "--- wake up and co2 heating now ---";
	eState = E_STATE::CO2_PREHEAT;
	digitalWrite(PIN_DIGITAL::DIO19,HIGH);
	uint32_t u32ct = CO2_WAIT;
	//電源供給だけ開始して再びスリープに
	the_twelite.sleep(u32ct,true,false);
}

// wakeup procedure
void wakeup() {
	if(eState != E_STATE::CO2_PREHEAT && eState != E_STATE::LONG_SLEEP){
		CO2_heat_wait();
	}
	if(eState == E_STATE::LONG_SLEEP){
		//夜間などのディープスリープ開けには端末をリセットして最初の受信待機状態へ
		the_twelite.reset_system();
	}

	Wire.begin();

	Serial	<< crlf << "--- " << APP_NAME << ":" << FOURCHARS << " wake up ---";

	if(eState != E_STATE::LONG_SLEEP){
		eState = E_STATE::INIT; // go into INIT state in the loop()
	}else{
		eState = E_STATE::RECIEVE_WAIT;
	}
}

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */