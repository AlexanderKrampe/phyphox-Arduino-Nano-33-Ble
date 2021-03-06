#ifndef PHYPHOXBLE_H
#define PHYPHOXBLE_H
#define NDEBUG

#include <mbed.h>
#include <ble/BLE.h>
#include <ble/GattServer.h>
#include <Gap.h>
#include <algorithm>
#include <UUID.h>
#include <string>
#include <AdvertisingParameters.h>
#include <AdvertisingDataBuilder.h>
#include <HardwareSerial.h>

#ifndef NDEBUG
using arduino::HardwareSerial;
#endif
using events::EventQueue; 
using rtos::Thread;
using mbed::callback;
using std::copy;

class BleServer : public ble::Gap::EventHandler
{
	private:
	/* Members every server need to have
	 * phyphox service uuid
	 * phyphox uuid for experiment transfer
	 * a buffer that hlds the data_packages which contans 20bytes
	 * an underlying BLE object 
	 * a characteristic to that phyphox subscribes named data char
	 */
	const UUID customServiceUUID = UUID("cddf0001-30f7-4671-8b43-5e40ba53514a");
	const UUID phyphoxUUID = UUID("cddf0002-30f7-4671-8b43-5e40ba53514a");
	uint8_t data_package[20] = {0};
	char DEVICE_NAME[20] = "Arduino";
	UUID dataOneUUID = UUID("59f51a40-8852-4abe-a50f-2d45e6bd51ac");
	
	/*BLE stuff*/
	BLE& ble = BLE::Instance(BLE::DEFAULT_INSTANCE);
    ReadWriteArrayGattCharacteristic<uint8_t, sizeof(data_package)> dataChar{phyphoxUUID, data_package, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY}; //Note: Use { } instead of () google most vexing parse
	uint8_t readValue[12] = {0};
	ReadOnlyArrayGattCharacteristic<uint8_t, sizeof(readValue)> readCharOne{dataOneUUID, readValue, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY};
	Thread ble_server, transfer;
	EventQueue queue{8 * EVENTS_EVENT_SIZE};
	/*end BLE stuff*/
	
	//helper function to initialize BLE server and for connection poperties
	void bleInitComplete(BLE::InitializationCompleteCallbackContext*);
	void when_disconnection(const Gap::DisconnectionCallbackParams_t *);
	void when_subscription_received(GattAttribute::Handle_t);
	void when_connected(const Gap::ConnectionCallbackParams_t *);
	virtual void onDisconnectionComplete(const ble::DisconnectionCompleteEvent&);
	virtual void onConnectionComplete(const ble::ConnectionCompleteEvent&);
   
	//helper functon that runs in the thread ble_server
	static void waitForEvent(BleServer*);
	static void transferExp(BleServer*);
	GattCharacteristic* characteristics[2] = {&readCharOne, &dataChar};
	GattService customService{customServiceUUID, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *)};
	
	//helper function to construct event handler from class function
	template<typename ContextType> FunctionPointerWithContext<ContextType> as_cb(void (BleServer::*member)(ContextType context)) 
	{
        return makeFunctionPointer(this, member);
	}
	
	#ifndef NDEBUG
	HardwareSerial* printer; //for debug purpose
	#endif
	uint8_t* data = nullptr; //this pointer points to the data the user wants to write in the characteristic
	uint8_t* p_exp = nullptr; //this pointer will point to the byte array which holds an experiment
	size_t exp_len = 0; //try o avoid this maybe use std::array or std::vector
	
	public:
	BleServer() {};
	BleServer(const char* s) {strcpy(DEVICE_NAME, s);};
	BleServer(const char* s, const char* uuid) :dataOneUUID(UUID(uuid))
	{
		strcpy(DEVICE_NAME, s);	
	}
	/* TODO: add constructor with UUID input */
	BleServer(const BleServer&) = delete; //there is no need to copy a BleServer once established
	BleServer &operator=(const BleServer&) = delete; //there is no need to assign a BleServer to a BleServer
	~BleServer() = default; //no dynamic memory allocation 
	void write(float&);
	void write(float&, float&, float&);
	void write(float&, float&);
	void start(uint8_t* p = nullptr, size_t n = 0); //start method if you specify your own experiment in form of a byte array
	//void start();
	#ifndef NDEBUG
	void begin(HardwareSerial*); //for debug purpose
	void output(const char*); //for debug purpose
	void output(const uint32_t);
	#endif
};


struct crc32
{
	static void generate_table(uint32_t(&table)[256])
	{
		uint32_t polynomial = 0xEDB88320;
		for (uint32_t i = 0; i < 256; i++) 
		{
			uint32_t c = i;
			for (size_t j = 0; j < 8; j++) 
			{
				if (c & 1) {
					c = polynomial ^ (c >> 1);
				}
				else {
					c >>= 1;
				}
			}
			table[i] = c;
		}
	}

	static uint32_t update(uint32_t (&table)[256], uint32_t initial, const uint8_t* buf, size_t len)
	{
		uint32_t c = initial ^ 0xFFFFFFFF;
		const uint8_t* u = static_cast<const uint8_t*>(buf);
		for (size_t i = 0; i < len; ++i) 
		{
			c = table[(c ^ u[i]) & 0xFF] ^ (c >> 8);
		}
		return c ^ 0xFFFFFFFF;
	}
};
#endif
