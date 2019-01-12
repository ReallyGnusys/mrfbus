#!/usr/bin/env python3
import argparse
import os



if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-chann_offset', required=False, type=int, default=0,help="radio channel offsets from nominal")

    args = parser.parse_args()



    print (repr(args))

    akey = os.urandom(16)

    hfile = open('mrf_kdata.h','w')
    hfile.write("#include  <stdint.h>\n")

    hfile.write("#define MRF_CHANN_OFFSET  %d\n"%args.chann_offset)

    #hfile.write("extern const uint16_t  *_mrf_key;\n")

    hfile.close()

    cfile = open('mrf_kdata.c','w')
    cfile.write("#include  <stdint.h>\n")

    cfile.write("extern const uint16_t  _mrf_key[8] = {")

    wn = 0
    for i in range(16):
        if (i%2)== 0:
            val = akey[i]
        else:
            val = val + (akey[i] * 256)

            if (wn%4) == 0:
                cfile.write("\n   ")
            if i < 15:
                cfile.write("0x%04x,"%val)
            else:
                cfile.write("0x%04x"%val)
            wn += 1

    cfile.write("\n};\n")

    cfile.close()
