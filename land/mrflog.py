import logging
import install

def mrf_log():
    return logging.getLogger(install.logger_name);

def mrflog_init(level = install.log_level):
    formatter = logging.Formatter('[%(asctime)s] %(levelname)s %(filename)s.%(lineno)d - %(message)s')
    alog = logging.getLogger(install.logger_name)
    hdlr = logging.FileHandler(install.logdir+install.mrflog)
    hdlr.setFormatter(formatter)    
    alog.addHandler(hdlr) 
    alog.setLevel(level)  
    return alog
    ch = logging.StreamHandler()
    ch.setFormatter(formatter)
    ch.setLevel(level)
    alog.addHandler(ch) 
    logging.addLevelName( logging.WARNING, "\033[1;31m%s\033[1;0m" % logging.getLevelName(logging.WARNING))
    logging.addLevelName( logging.ERROR, "\033[1;41m%s\033[1;0m" % logging.getLevelName(logging.ERROR))
    return alog
   

