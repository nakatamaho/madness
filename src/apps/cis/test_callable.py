#!/usr/bin/env python3

import sys
import subprocess
sys.path.append("@CMAKE_SOURCE_DIR@/bin")
from madjsoncompare import madjsoncompare

if __name__ == "__main__":

    # some user output
    print("Testing @BINARY@/@TESTCASE@")
    print(" reference files found in directory: @SRCDIR@")

    prefix='mad_@BINARY@_@TESTCASE@'
    outputfile=prefix+'.calc_info.json'
    referencefile="@SRCDIR@/"+prefix+".calc_info.ref.json"

    # run test
    global_arguments=' --geometry=he'
    dft_arguments=' --dft="maxiter=1; econv=1.e-4; dconv=1.e-3; prefix='+prefix+'"'
    other_arguments=' --response="thresh=1.e-3; maxiter=10; guess_maxiter=0; econv=1; dconv=1; guess_excitation_operators=dipole+; excitations=0"'
    cmd='rm '+outputfile+'; @BINARY@ '+global_arguments + dft_arguments  + other_arguments
    print("executing \n ",cmd)
    p=subprocess.run(cmd,shell=True,capture_output=True, text=True)
    print("finished with run")
    print(p.stdout)
    exitcode=p.returncode
    print("exitcode ",exitcode)

    # compare results
    # cmp=madjsoncompare(outputfile,referencefile)
    # cmp.compare(["cis_excitations",0,"irrep"],1.0)
    # cmp.compare(["cis_excitations",0,"omega"],1.e-3)
    # print("final success: ",cmp.success)

    sys.exit(p.returncode)