import json
import sys
import os.path
import io
from os import path
from PIL import Image

def main():
    folder = sys.argv[1]
    print('Processing '+folder)
    with open(folder+'/metadata.json') as f:
        meta = json.load(f)
    for zl in meta['tile_matrix']:
        xc = zl['origin'][0]
        yc = zl['origin'][1]
        xp = zl['pixel_size'][0]
        yp = zl['pixel_size'][1]
        for x in range(zl['matrix_size'][0]):
            yc = zl['origin'][1]
            for y in range(zl['matrix_size'][1]):
                filename = folder+'/'+zl['id']+'/'+str(x)+'/'+str(y)+'.png'
                if path.exists(filename):
                    fileprops = {}
                    im=Image.open(filename)
                    fileprops['size'] = im.size
                    fileprops['bounds'] = [xc,yc,xc+im.size[0]*xp,yc+im.size[1]*yp]
                    xn = xc + im.size[0]*xp
                    yc = yc + im.size[1]*yp
                    with io.open(filename+'.json', 'w', encoding='utf-8') as f:
                        f.write(json.dumps(fileprops, ensure_ascii=False))
            xc = xn
                   

        

main()