# uncompyle6 version 3.7.4
# Python bytecode 3.7 (3394)
# Decompiled from: Python 3.7.5 (tags/v3.7.5:5c02a39a0b, Oct 15 2019, 00:11:34) [MSC v.1916 64 bit (AMD64)]
# Embedded file name: astc_conv.py
from PIL import Image, ImageChops
import array, tempfile, os, struct

def astc_dims(fmt):
    if isinstance(fmt, int):
        return astc_dims({37808:'COMPRESSED_RGBA_ASTC_4x4_KHR', 
         37809:'COMPRESSED_RGBA_ASTC_5x4_KHR', 
         37810:'COMPRESSED_RGBA_ASTC_5x5_KHR', 
         37811:'COMPRESSED_RGBA_ASTC_6x5_KHR', 
         37812:'COMPRESSED_RGBA_ASTC_6x6_KHR', 
         37813:'COMPRESSED_RGBA_ASTC_8x5_KHR', 
         37814:'COMPRESSED_RGBA_ASTC_8x6_KHR', 
         37815:'COMPRESSED_RGBA_ASTC_8x8_KHR', 
         37816:'COMPRESSED_RGBA_ASTC_10x5_KHR', 
         37817:'COMPRESSED_RGBA_ASTC_10x6_KHR', 
         37818:'COMPRESSED_RGBA_ASTC_10x8_KHR', 
         37819:'COMPRESSED_RGBA_ASTC_10x10_KHR', 
         37820:'COMPRESSED_RGBA_ASTC_12x10_KHR', 
         37821:'COMPRESSED_RGBA_ASTC_12x12_KHR'}[fmt])
    dims = str(fmt).split('_')[3]
    return [int(c) for c in dims.split('x')]


def tile(f):
    _, w, h, _, iw, _, ih, _, _, _ = struct.unpack('<IBBBHBHBHB', f.read(16))
    bw, bh = (iw + (w - 1)) // w, (ih + (h - 1)) // h
    d = f.read()
    return (bw, bh, tile2(d, bw, bh))


def tile2(d, bw, bh):
    assert len(d) == 16 * bw * bh
    fe = [d[i:i + 16] for i in range(0, len(d), 16)]
    assert len(fe) == bw * bh
    r = []
    for j in range(0, bh - 1, 2):
        for i in range(0, bw, 2):
            if i < bw - 1:
                r += [
                 fe[(bw * j + i)],
                 fe[(bw * (j + 1) + i)],
                 fe[(bw * (j + 1) + (i + 1))],
                 fe[(bw * j + (i + 1))]]
            else:
                r += [
                 fe[(bw * j + i)],
                 fe[(bw * (j + 1) + i)]]

    if bh & 1:
        r += fe[bw * (bh - 1):]
    assert len(r) == bh * bw
    return (b'').join(r)


def pad(im, mult):
    w = (im.size[0] + (mult - 1)) // mult * mult
    n = Image.new('RGBA', (w, im.size[1]))
    n.paste(im, (0, 0))
    return n


def round_up(n, d):
    return d * ((n + d - 1) // d)


def convert(im, fmt='COMPRESSED_RGBA_ASTC_4x4_KHR', effort='thorough', astc_encode='astcenc', astc_opt=''):
    w, h = astc_dims(fmt)
    ni = Image.new('RGBA', (round_up(im.size[0], w), round_up(im.size[1], h)))
    ni.paste(im, (0, 0))
    png = tempfile.NamedTemporaryFile(delete=False)
    #ni.transpose(Image.FLIP_TOP_BOTTOM).save(png, 'png')
    ni.save(png, 'png')
    png.close()
    astc = tempfile.NamedTemporaryFile(suffix='.astc',delete=False)
    os.system('"%s" -cl %s %s %dx%d -%s -silent %s' % (astc_encode, png.name, astc.name, w, h, effort, astc_opt))
    bw, bh, d0 = tile(open(astc.name, 'rb'))
    astc.close()
    if os.path.exists(astc.name):
        os.unlink(astc.name)
    if os.path.exists(png.name):
        os.unlink(png.name)
    stride = 16 * bw
    dd = array.array('B', d0)
    return (
     stride, ni.size, dd)