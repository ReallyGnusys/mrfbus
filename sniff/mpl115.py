def comp(val,bits):
    comp = 0
    for i in range ( 0,bits):
        if not val & 1:
            comp += pow(2, i )
        val = val >> 1
    return comp


def twoscomp(val,bits):
    return comp(val,bits) + 1



def mpl115coeff(val,bits,intbits,zpbits):
    sign = 1.0
    if  val > pow(2,bits-1):
        val = twoscomp(val,bits)
        sign = -1.0
    coeff = sign * val / pow(2.0,bits-intbits+zpbits-1)
    return coeff

        
    
def coeff_a0(val):
    return mpl115coeff(val,16,12,0)

def coeff_b1(val):
    return mpl115coeff(val,16,2,0)

def coeff_b2(val):
    return mpl115coeff(val,16,1,0)

def coeff_c12(val):
    return mpl115coeff(val,16,0,9)


def mpa_kpa(pmeas,tmeas):
    a0 = coeff_a0(0x33fd)
    b1 = coeff_b1(0xc24a)
    b2 = coeff_b2(0xc972)
    c12 = coeff_c12(0x2ca0)
    padc = pmeas / pow(2.0,6.0)
    tadc = tmeas / pow(2.0,6.0)

    if 1:
        print "mpa_kpa ( %X , %X ) "%(pmeas,tmeas)
        print "ao %X  b1 %X b2 %X c12 %X"%(a0,b1,b2,c12)
        print "padc %f  tadc %f"%(padc,tadc)
    pcomp = a0 + ( b1 +  c12 * tadc)*padc
    pcomp += b2 * tadc
    kpa = 50.0 + pcomp * ( 115.0 -50.0)/1023.0
    return kpa    

def sht_temp(tmeas):
    return -46.85 + 175.72 * tmeas /pow(2.0,16.0)

def sht_rh (rhmeas):
    return -6 + 125.0*rhmeas/pow(2.0,16.0)

