import os
import sys
import shutil
import argparse
import pathlib
import logging
import subprocess

TIMEOUT_SECONDS = 30

LOGGER_TEMPLATE = "STDOUT >>>>>> \n %s \n <<<<<< \n" \
                  "STDERR >>>>>> \n %s \n <<<<<<"

def run_target(prog :pathlib.Path, rsp :pathlib.Path, timeout :int =TIMEOUT_SECONDS):
    if os.access(prog, os.X_OK) is False:
        raise RuntimeError("Invalid program: {}".format(repr(prog)))
    if os.access(rsp, os.R_OK) is False:
        raise RuntimeError("Invalid response file: {}".format(repr(rsp)))
    try:
        p_ = subprocess.Popen([str(prog.absolute()), "@" + str(rsp.absolute())], 
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        outs, errs = p_.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        p_.kill()
        outs, errs = p_.communicate()
        return None, outs, errs
    else:
        return p_.returncode, outs, errs

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("ALPHA", help="Implementation Alpha",
                        type=pathlib.Path)
    parser.add_argument("BRAVO", help="Implementation Bravo",
                        type=pathlib.Path)
    parser.add_argument("-i", help="Dir path of where the inputs locate", 
                        type=pathlib.Path, required=True)
    parser.add_argument("-o", help="Dir of outputs, need to create some tmp files there",
                        type=pathlib.Path, required=True)
    args = parser.parse_args()
    
    logger = logging.getLogger("REGRESSION")
    logger.setLevel("INFO")
    _fmt = logging.Formatter("%(asctime)s,%(msecs)d %(filename)s:%(lineno)d "
                             "%(levelname)s: %(message)s", datefmt="%y.%m.%d %H:%M:%S")
    _hdl = logging.StreamHandler(sys.stdout)
    _hdl.setFormatter(_fmt)
    logger.addHandler(_hdl)
    _hdl = logging.FileHandler(args.o / "run.output", mode="w")
    _hdl.setFormatter(_fmt)
    logger.addHandler(_hdl)
    
    for i in args.i.iterdir():
        if i.is_file() is False:
            logger.warning("Ignore %s", i)
            continue
        else:
            logger.info("Reading %s", i)

        i_ = args.o / "run.input"
        shutil.copyfile(i, i_)

        ra, oa, ea = run_target(args.ALPHA, i_)
        if ra is None:
            logger.warning("ALPHA: timeout; " + LOGGER_TEMPLATE, oa, ea)
        else:
            logger.info("ALPHA: ret %d; " + LOGGER_TEMPLATE, ra, oa, ea)

        rb, ob, eb = run_target(args.BRAVO, i_)
        if rb is None:
            logger.warning("BRAVO: timeout; " + LOGGER_TEMPLATE, ob, eb)
        else:
            logger.info("BRAVO: ret %d; " + LOGGER_TEMPLATE, rb, ob, eb)

        logger.info("**RESULT** : RETCODE %s, STDOUT %s, STDERR %s",
            True if ra == rb else False,
            True if oa == ob else False,
            True if ea == eb else False,
        )

        os.unlink(i_)

        stats = 0
        stats |= (1<<0) if ra != rb else 0
        stats |= (1<<1) if oa != ob else 0
        stats |= (1<<2) if ea != eb else 0
        if stats > 0:
            shutil.copyfile(i, args.o / "x{:02x}-{:s}".format(stats, i.name))
            logger.warning("Found inconsistency, saved input")
        else:
            logger.info("Go for next")

    logger.info("Done")
