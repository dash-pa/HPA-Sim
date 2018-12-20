#!/usr/bin/python

from __future__ import print_function
import gattlib, os, time, struct, sys, serial, traceback
import xml.etree.ElementTree as xmlET

FATAL_LOG_FILE = '/boot/sumpac/crashlog.txt'

CONFIG_FILE = '/boot/sumpac/config.xml'
SUPPORTED_CONF_VERSION = (1, 0) # (major, minor), aka '1.0' = (1, 0)

# Serial port configuration (no parity, 1 stop bit, 8 data bits)
SERIAL_PORT = '/dev/ttyAMA0'
BAUD_RATE = 115200

# Load in config XML
print('Loading config file...')
fatalFile = open(FATAL_LOG_FILE, 'w+')
try:
    tree = xmlET.parse(CONFIG_FILE)
    root = tree.getroot()
    
    conf_ver_major, conf_ver_minor = list(int(x) for x in root.find('version').text.split('.'))
    # Major versions must match, minor version must be equal or greater
    if not (conf_ver_major == SUPPORTED_CONF_VERSION[0]):
        raise Exception('Expected major version =%d, but got %d.' % (SUPPORTED_CONF_VERSION[0], conf_ver_major))
    if not (conf_ver_minor >= SUPPORTED_CONF_VERSION[1]):
        raise Exception('Expected minor version >=%d, but got %d.' % (SUPPORTED_CONF_VERSION[1], conf_ver_minor))
    
    
    # The BLE address of the Vector 3 pedals
    # find addr via hcitool
    BLE_DEVICE_ADDRESS = root.find('ble_dev_addr').text
    # The Bluez handle of the cycling power characteristic (powChar assigned number: 0x2a63)
    # Find with `sudo gatttool -t random -b $BLE_DEVICE_ADDRESS --char-desc | grep -i 2a63`
    POWER_CHARACTERISTIC_HANDLE = eval(root.find('char_handle_power').text)

    print('Config loaded and valid.')
    print('BLE dev addr:', BLE_DEVICE_ADDRESS)
    print('pwr char handle:', POWER_CHARACTERISTIC_HANDLE)
except:
    traceback.print_exc()
    print('[%s] Crashed trying to load config file "%s". Traceback: \n%s\n' % (time.ctime(), CONFIG_FILE, traceback.format_exc()), file=fatalFile)
    fatalFile.close()
    sys.exit(1)

    
# Structure of packets sent over serial port
def watts2packet(watts):
    return struct.pack('<HHHH', 0xFFFF, watts, (-watts) & 0xFFFF, 0xBEEF)


def bytes2hex(bs):
    s = ''
    for c in bs:
        s += '%02x' % ord(c)
    return s

def quit(val):
    if len(sys.argv) == 2:
        os.remove(sys.argv[1])
    sys.exit(val)

class Requester(gattlib.GATTRequester):
    def __init__(self, ble_addr, serial_port):
        # idk what the False is, don't connect yet?
        gattlib.GATTRequester.__init__(self, ble_addr, False)
        self.__ser = serial_port
    # Called whenever there's a BLE notification
    def on_notification(self, handle, data):
        try:
            if handle != POWER_CHARACTERISTIC_HANDLE:
                print('Notification from unknown handle: 0x%04x' % handle)
                return
            #print('notification from 0x%02x with data:' % handle, bytes2hex(data))
            data = data[3:] # strip off 1b2800
            #print('data (0x%02x%02x) translates to %d watts' % (ord(data[2]), ord(data[3]), struct.unpack('<H', data[2:4])[0]))
            watts = struct.unpack('<H', data[2:4])[0]
            self.__ser.write(watts2packet(watts))
            self.__ser.flush()
            print('<BLE> %d watts' % watts)
        except Exception as e:
            print('Failed to get wattage from data:')
            print(bytes2hex(data))
            raise e

def getRequester(ble_addr, serial_port):
    print('Getting requester...')
    while True:
        try:
            print('Getting requester...')
            reqer = Requester(ble_addr, serial_port)
            print('Got requester.')
            return reqer
        except KeyboardInterrupt:
            print('User quit.')
            quit(1)
        except Exception as e:
            print('Failed, traceback follows. Probably adapter not available. Retrying in 5s.')
            traceback.print_exc()
            time.sleep(5)

def connect(requester):
    while True:
        try:
            if not requester.is_connected():
                print('Connecting...')
                # wait=True
                try:
                    requester.connect(True, channel_type='random')
                except Exception as e:
                    print('Connection failed, traceback follows. Disconnecting and trying again in 1s.')
                    traceback.print_exc()
                    requester.disconnect()
                    time.sleep(1)
                    continue
                print('Connected.')
            else:
                print('Already connected.')
            print('Enabling notifications...')
            reqer.write_by_handle(
                POWER_CHARACTERISTIC_HANDLE + 1,
                str(bytearray( [0x01, 0x00] ))
            )
            print('Notifications enabled.')
            return
        except KeyboardInterrupt:
            print('User quit.')
            quit(1)
        except Exception as e:
            print('Failed, traceback follows. Retrying in 3s.')
            traceback.print_exc()
            time.sleep(3)

# definitions done, program start
while True:
    try:
        print('Starting... (note: SU perms required)')
        serial_port = serial.Serial(
            port=SERIAL_PORT,
            baudrate=BAUD_RATE,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS,
        )
        print('Got serial port.')
        '''
        print('Doing fake data loop forever')
        watts = 0
        while True:
            serial_port.write(watts2packet(watts))
            watts += 10
            time.sleep(1)
        '''
        reqer = getRequester(BLE_DEVICE_ADDRESS, serial_port)
        connect(reqer)
        while True:
            if not reqer.is_connected():
                connect(reqer)
            time.sleep(1)
    except KeyboardInterrupt:
        print('User quit.')
        quit(1)
    except Exception as e:
        print('Unknown error as follows. Restarting everything in 10s.')
        traceback.print_exc()
        time.sleep(10)
