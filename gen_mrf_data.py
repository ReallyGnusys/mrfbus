#!/usr/bin/env python3
import argparse
import os



if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-chann_offset', required=False, type=int, default=0,help="radio channel offsets from nominal")

    args = parser.parse_args()



    print (repr(args))

    akey = os.urandom(16)

    hfile = open('mrf_data.h','w')
    hfile.write("#include  <stdint.h>\n")

    hfile.write("#define MRF_CHANN_OFFSET  %d\n"%args.chann_offset)

    hfile.write("extern const uint8_t  *_mrf_key;\n")

    hfile.close()

    cfile = open('mrf_data.c','w')
    cfile.write("#include  <stdint.h>\n")

    cfile.write("const uint8_t  _mrf_key[16] = {")

    for i in range(16):
        if (i%4) == 0:
            cfile.write("\n   ")
        cfile.write("0x%02x,"%akey[i])
    cfile.write("\n};\n")

    cfile.close()
