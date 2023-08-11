import json
import sys
import os.path
import io
import astc_conv
from os import path
from PIL import Image

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
    

def loadImage(filenameBase,x,y):
    filename = filenameBase+str(x)+'/'+str(y)+'.png'
    if path.exists(filename):
        im=Image.open(filename)
        return im
    return None

def main():
    folder = sys.argv[1]
    output = sys.argv[2]
    print('Processing '+folder)
    tmeta = {}
    position = 0
    with open(folder+'/metadata.json') as f:
        meta = json.load(f)
    tmeta['name'] = meta['name']
    tmeta['bounds'] = [meta['extent'][0],meta['extent'][3],meta['extent'][2],meta['extent'][1]]
    tmeta['tile_matrix'] = []
    with io.open(output, 'wb') as o:    
        for zl in meta['tile_matrix']:
            zitem = {}
            zitem['id'] = zl['id']
            zitem['bounds'] = [zl['origin'][0],zl['origin'][1],zl['extent'][2],zl['extent'][1]]
            zitem['images'] = []
            zitem['pixel_size'] = zl['pixel_size']
            zitem['tile_size'] = [zl['pixel_size'][0]*zl['tile_size'][0],zl['pixel_size'][1]*zl['tile_size'][1]]
            xc = zl['origin'][0]
            yc = zl['origin'][1]
            xp = zl['pixel_size'][0]
            yp = zl['pixel_size'][1]
            fmt = 37808
            hasImages = False
            if zl['matrix_size'][0]*zl['matrix_size'][1] >=8:
                fmt = 37812
            for x in range(zl['matrix_size'][0]):
                for y in range(zl['matrix_size'][1]):
                    filenameBase = folder+'/'+zl['id']+'/'
                    filename = filenameBase+str(x)+'/'+str(y)+'.png'
                    if path.exists(filename):
                        im=Image.open(filename)
                        expand = 1
                        nim = Image.new('RGBA',(im.size[0]+expand*2,im.size[1]+expand*2))
                        nim.putalpha(0)
                        nim.paste(im,(expand,expand))
                        
                        pim = loadImage(filenameBase,x,y-1)
                        if pim!=None:
                            nim.paste(pim.crop((0,pim.size[1]-expand,pim.size[0],pim.size[1])),(expand,0))

                        pim = loadImage(filenameBase,x,y+1)
                        if pim!=None:
                            nim.paste(pim.crop((0,0,pim.size[0],expand)),(expand,nim.size[1]-expand))

                        pim = loadImage(filenameBase,x-1,y)
                        if pim!=None:
                            nim.paste(pim.crop((pim.size[0]-expand,0,pim.size[0],pim.size[1])),(0,expand))

                        pim = loadImage(filenameBase,x+1,y)
                        if pim!=None:
                            nim.paste(pim.crop((0,0,expand,pim.size[1])),(nim.size[0]-expand,expand))

                        rsize = im.size
                        im = nim
                        stride,asize,astc = astc_conv.convert(im,fmt)
                        if len(astc)>0:
                            hasImages = True
                            fileinfo = {}
                            fileinfo['size'] = im.size
                            fileinfo['bounds'] = [xc+x*rsize[0]*xp-xp*expand,yc+y*rsize[1]*yp-yp*expand,xc+(x+1)*rsize[0]*xp+xp*expand,yc+(y+1)*rsize[1]*yp+yp*expand]
                            fileinfo['x'] = x
                            fileinfo['y'] = y
                            fileinfo['address'] = position
                            fileinfo['image_stride'] = stride
                            fileinfo['image_size'] = len(astc)
                            fileinfo['image_fmt'] = fmt
                            o.write(astc)
                            with open(filename+'.bin','wb') as f:
                                f.write(astc)
                            position += len(astc)
                            pad(o,64)
                            position += 64
                            position += alignFile(o,position,64)
                            zitem['images'].append(fileinfo)
            if hasImages==True:
                tmeta['tile_matrix'].append(zitem)
        alignFile(o,position,4096)
    with io.open(output+'.json', 'w', encoding='utf-8') as f:
        f.write(json.dumps(tmeta, indent=4))
                   

        

main()