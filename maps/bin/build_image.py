import json
import msgpack
import struct
import sys
import os.path
import io
from os import path

def pad(o, m):
    if m>0 :
        barray = bytearray(m)
        for i in range(0,m):
            barray[i] = 0xFF
        o.write(barray)
    return


def alignFile(o, position, aligment):
    m = position % aligment
    if m>0 :
        m = aligment-m
        barray = bytearray(m)
        for i in range(0,m):
            barray[i] = 0xFF
        o.write(barray)
    return m
    

def main():
    output = sys.argv[1]
    maps = []
    maps_meta = []
    position = 0
    with io.open(output, 'wb') as o:    
        for i, arg in enumerate(sys.argv):
            if i>1:
                maps.append(arg)
        for map in maps:
            with open(map, mode='rb') as file:
                mapContent = file.read()
            with open(map+'.json') as f:
                mapMeta = json.load(f)
            mapMeta["base_address"] = position
            o.write(mapContent)
            position += len(mapContent)
            position += alignFile(o,position,4096)
            maps_meta.append(mapMeta)

    with io.open(output+'.json', 'w') as f:
        f.write(json.dumps(maps_meta,indent=6))
      
    with io.open(output+'.dat', 'wb') as f:
        cnt = len(maps_meta)
        f.write(struct.pack('<b',cnt))
        i = 0
        pos = 1+cnt*60
        for map in maps_meta:
            f.seek(pos)
            baseAddress = map['base_address']
            f.write(struct.pack('<b',len(map['tile_matrix'])))
            for zl in map['tile_matrix']:
                b = zl['bounds']
                ps = zl['pixel_size']
                ts = zl['tile_size']
                f.write(struct.pack('<H4f4f',int(zl['id']),b[0],b[1],b[2],b[3],ps[0],ps[1],ts[0],ts[1]))
                f.write(struct.pack('<H',len(zl['images'])))
                for image in zl['images']:
                   s = image['size'] 
                   b = image['bounds']
                   f.write(struct.pack('<2I6H2f4f',image['address']+baseAddress,image['image_size'],s[0],s[1],image['image_stride'],image['image_fmt'],image['x'],image['y'],b[2]-b[0],b[1]-b[3],b[0],b[1],b[2],b[3]))
            endPos = f.seek(0,io.SEEK_CUR)
                
            f.seek(1+i*60)
            b = map['bounds']
            f.write(struct.pack('<32s4fIII',bytes(map['name'], 'utf-8'),b[0],b[1],b[2],b[3],map['base_address'],pos,endPos-pos))
            pos = endPos
            i+=1

main()