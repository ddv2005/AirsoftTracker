import argparse
import json
import sys
import os.path
import io
from os import path
import serial
import time
import struct
import ntpath

show_r = False
PMC_SCREEN_FLASH_START  = 1
PMC_SCREEN_FLASH_START_RESPONSE=2
PMC_SCREEN_FLASH_END    =3
PMC_SCREEN_FLASH_WRITE  =4
PMC_SCREEN_FLASH_WRITE_COMPRESSED  =14
PMC_SCREEN_FLASH_READ   =5
PMC_SCREEN_FLASH_READ_RESPONSE   =6
PMC_SCREEN_FLASH_META_START =7
PMC_SCREEN_FLASH_META_DATA  =8
PMC_SCREEN_FLASH_META_END   =9
PMC_SCREEN_FLASH_FILE_START =10
PMC_SCREEN_FLASH_FILE_DATA  =11
PMC_SCREEN_FLASH_FILE_END   =12

PMC_SCREEN_FLASH_EXIT   =99
PMC_SCREEN_FLASH_ACK    =100
PMC_SCREEN_FLASH_ERR    =101
SERIAL_APP_1=0x85
SERIAL_APP_2=0x3B
SERIAL_APP_3=0xDE
SERIAL_APP_4=0x02

def csum(dd,extra):
    result = extra & 0xFF
    for b in dd:
        result ^= (b & 0xFF)
    return result

def readPacket(ser,timeout=2):
    global show_r
    packet = bytearray([])
    readState = 0
    size = 0
    startTime = time.time()
    while True:
        if ser.inWaiting()>0:
            b = ser.read(1)[0]
            if(show_r==True):
                print(hex(b), end ="", flush=True)
                #print(bytearray([b]).decode('ascii'), end ="", flush=True)
            if readState==0:
                if b==SERIAL_APP_1:
                    readState = 1
            elif readState==1:
                if b==SERIAL_APP_2:
                    readState = 2
                else:
                    readState = 0
            elif readState==2:
                if b==SERIAL_APP_3:
                    readState = 3
                else:
                    readState = 0
            elif readState==3:
                if b==SERIAL_APP_4:
                    readState = 4
                else:
                    readState = 0
            elif readState==4:
                size = b
                readState = 5
            elif readState==5:
                size = size + (b<<8)
                if size<8192:
                    readState = 6
                else:
                    readState = 0
            elif readState==6:
                if len(packet)<(size-1):
                    packet.extend([b])
                else:
                    cs = csum(packet,0)
                    if b==cs:
                        return packet
                    else:
                        print('BAD CS '+hex(cs)+' '+hex(b)+' '+packet.hex())
                        return bytearray([])
        else:
            time.sleep(0.001)
        if(time.time()-startTime)>=timeout:
            print('Timeout')
            #show_r = True
            return bytearray([])

    return bytearray([])

def createSerialPacket(cmd,dd):
    result = bytearray([SERIAL_APP_1,SERIAL_APP_2,SERIAL_APP_3,SERIAL_APP_4])
    s = len(dd)+2
    result.extend([s & 0xFF,(s>>8) & 0xFF,cmd])
    result.extend(dd)
    result.extend([csum(dd,cmd)])
    #print(result.hex())
    return result


def writeImage(ser,imagedata,metadata):
    print("Writing image...")
    ser.write(createSerialPacket(PMC_SCREEN_FLASH_START,bytearray([])))
    packet = readPacket(ser)
    if len(packet)>0:
        if packet[0]==PMC_SCREEN_FLASH_START_RESPONSE:
            s = struct.Struct('<BI')
            resp = s.unpack(packet)
            print("Start flash result "+str(resp[1]))
            if resp[1]==0:
                print("Writing flash")
                cnt = int(len(imagedata)/4096)
                sector = bytearray(4096+6)
                sector[0] = 00
                sector[1] = 0x10
                for i in range(cnt):
                    for j in range(4096):
                        if(j%2==0):
                            sector[j+6] = imagedata[i*4096+j] ^ 0xAA
                        else:
                            sector[j+6] = imagedata[i*4096+j] ^ 0x55
                    address = 4096+i*4096
                    sector[2] = address & 0xFF
                    sector[3] = (address >> 8) & 0xFF
                    sector[4] = (address >> 16) & 0xFF
                    sector[5] = (address >> 24) & 0xFF
                    while 1:
                        print("Sector "+str(cnt)+"/"+str(i)+".", end ="", flush=True)
                        packet = createSerialPacket(PMC_SCREEN_FLASH_WRITE_COMPRESSED,sector)
                        spackets = [packet[i:i+1512] for i in range(0, len(packet), 1512)]
                        #spackets = [packet[i:i+32] for i in range(0, len(packet), 32)]
                        for p in spackets:
                            ser.write(p)
                            ser.flush()
                            time.sleep(0.0001)
                        packet = readPacket(ser)
                        if len(packet)>0:
                            if packet[0]!=PMC_SCREEN_FLASH_ACK:
                                print("ERR "+hex(packet[1]))
                            else:
                                print("OK")
                                break
                            time.sleep(0.02)
        ser.write(createSerialPacket(PMC_SCREEN_FLASH_END,bytearray([])))
        readPacket(ser)
    else:
        print("Error") 
    print("Writing meta...")
    ser.write(createSerialPacket(PMC_SCREEN_FLASH_META_START,bytearray([])))
    packet = readPacket(ser)
    if len(packet)>0:
        if packet[0]==PMC_SCREEN_FLASH_ACK:

            cnt = len(metadata)
            i = 0
            while cnt>0:
                if cnt>512:
                    s = 512
                else:
                    s = cnt
                cnt -= s
                sector = metadata[i:(i+s)]
                while 1:
                    print("Sector "+str(i)+".", end ="", flush=True)
                    packet = createSerialPacket(PMC_SCREEN_FLASH_META_DATA,sector)
                    ser.write(packet)
                    ser.flush()
                    packet = readPacket(ser)
                    if len(packet)>0:
                        if packet[0]!=PMC_SCREEN_FLASH_ACK:
                            print("ERR")
                        else:
                            print("OK")
                            break
                        time.sleep(0.01)      
                i += s      
            ser.write(createSerialPacket(PMC_SCREEN_FLASH_META_END,bytearray([])))
            packet = readPacket(ser)
        else:
            print("Error")

    print("Done.")
    return

def writeFile(ser,imagedata,filename):
    print("Writing file...")
    ser.write(createSerialPacket(PMC_SCREEN_FLASH_FILE_START,bytearray(filename.encode())+ b'\x00'))
    packet = readPacket(ser)
    if len(packet)>0:
        if packet[0]==PMC_SCREEN_FLASH_ACK:

            cnt = len(imagedata)
            i = 0
            while cnt>0:
                if cnt>512:
                    s = 512
                else:
                    s = cnt
                cnt -= s
                sector = imagedata[i:(i+s)]
                while 1:
                    print("Sector "+str(i)+".", end ="", flush=True)
                    packet = createSerialPacket(PMC_SCREEN_FLASH_FILE_DATA,sector)
                    ser.write(packet)
                    ser.flush()
                    packet = readPacket(ser)
                    if len(packet)>0:
                        if packet[0]!=PMC_SCREEN_FLASH_ACK:
                            print("ERR")
                        else:
                            print("OK")
                            break
                        time.sleep(0.01)      
                i += s      
            ser.write(createSerialPacket(PMC_SCREEN_FLASH_FILE_END,bytearray([])))
            packet = readPacket(ser)
        else:
            print("Error")

    print("Done.")
    return

def readImage(ser,size):
    print("Reading image...")
    result = bytearray([])
    ser.write(createSerialPacket(PMC_SCREEN_FLASH_START,bytearray([])))
    packet = readPacket(ser)
    if len(packet)>0:
        if packet[0]==PMC_SCREEN_FLASH_START_RESPONSE:
            s = struct.Struct('<BI')
            resp = s.unpack(packet)
            print("Start flash result "+str(resp[1]))
            if resp[1]==0:
                print("Reading flash")
                cnt = int(size*1024/4096)
                sector = bytearray(6)
                sector[0] = 00
                sector[1] = 0x10
                for i in range(cnt):
                    address = 4096+i*4096
                    sector[2] = address & 0xFF
                    sector[3] = (address >> 8) & 0xFF
                    sector[4] = (address >> 16) & 0xFF
                    sector[5] = (address >> 24) & 0xFF
                    while 1:
                        print("Sector "+str(i)+".", end ="", flush=True)
                        packet = createSerialPacket(PMC_SCREEN_FLASH_READ,sector)
                        ser.write(packet)
                        packet = readPacket(ser)
                        if len(packet)>0:
                            if packet[0]!=PMC_SCREEN_FLASH_READ_RESPONSE:
                                print("ERR")
                            else:
                                print("OK")
                                result.extend(packet[1:])
                                break
        ser.write(createSerialPacket(PMC_SCREEN_FLASH_END,bytearray([])))
    else:
        print("Error")    
    print("Done.")
    return result


def main():
    parser = argparse.ArgumentParser(description='Airsoft Tracker Maps Image Tool')
    parser.add_argument('image', help='Image file name')
    parser.add_argument('-c','--comm', required=False, help='COM port')
    parser.add_argument('-r','--read',action='store_true', help='Read image')
    parser.add_argument('-s','--size',type=int, default=16*1024, help='Read size in KB')
    parser.add_argument('-w','--wfile',action='store_true', help='Write file')
    args = parser.parse_args()
    comm = args.comm
    if not comm:
        print("Looking for COM port")
        import serial.tools.list_ports as list_ports
        for each_port in reversed(sorted(list_ports.comports())):
            if each_port[1].find("Silicon Labs")>=0:
                comm = each_port[0]
                print("Comm port found "+comm)
                break

    imagefile = args.image
    readSize = args.size
    ser = serial.Serial(comm,baudrate=115200)
    ser.set_buffer_size(rx_size = 8192, tx_size=256)

    print("Connecting", end ="", flush=True)
    while True:
        if ser.inWaiting()>0:
            ser.read(ser.inWaiting())
        print(".", end ="", flush=True)
        values = bytearray([0x92,0x5A])
        ser.write(values)
        startTime = time.time()
        while (time.time()-startTime)<2:
            if ser.inWaiting()>0:
                b = ser.read(1)[0]
                #print(bytearray([b]).decode('ascii'), end ="", flush=True)
                if b==0x07:
                    print("")
                    print("Connected")
                    time.sleep(0.01)
                    values = bytearray([0x07])
                    ser.write(values)
                    ser.baudrate = 921600
                    time.sleep(1)

                    if args.read==True:
                        imagedata = readImage(ser,readSize)
                        with open(imagefile,"wb") as f:
                            f.write(imagedata)
                    else:
                        if args.wfile==True:
                            with open(imagefile,"rb") as f:
                                imagedata = f.read()
                            writeFile(ser,imagedata,ntpath.basename(imagefile))
                        else:
                            with open(imagefile,"rb") as f:
                                imagedata = f.read()
                            with open(imagefile+'.dat',"rb") as f:
                                metadata = f.read()
                            writeImage(ser,imagedata,metadata)

                    ser.write(createSerialPacket(PMC_SCREEN_FLASH_EXIT,bytearray([])))
                    return
            else:
                time.sleep(0.01)
        

main()