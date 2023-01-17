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
    global_arguments='  --geometry=h2o'
    dft_arguments=' --dft="k=8; localize=canon; prefix='+prefix+'"'
    other_arguments=' --response="freeze=1; thresh=1.e-3; econv=1.e-3; dconv=1.e-2"'
    cmd='rm '+outputfile+'; cis '+global_arguments + dft_arguments  + other_arguments
    print("executing \n ",cmd)
    output=subprocess.run(cmd,shell=True,capture_output=True, text=True).stdout
    print("finished with run")
    print(output)

    # compare results
    cmp=madjsoncompare(outputfile,referencefile)
    cmp.compare(["cis_excitations",0,"irrep"],1.0)
    cmp.compare(["cis_excitations",0,"omega"],1.e-4)
    cmp.compare(["cis_excitations",0,"oscillator_strength_length"],1.e-3)
    cmp.compare(["cis_excitations",0,"oscillator_strength_velocity"],1.e-3)

    referencefile="@SRCDIR@/"+prefix+"1.calc_info.ref.json"


    dft_arguments=' --dft="k=8; localize=canon; prefix='+prefix+'; no_compute=1"'
    other_arguments=' --response="irrep=a2; freeze=1; thresh=1.e-3; econv=1.e-3; dconv=1.e-2"'
    cmd='cis '+global_arguments + dft_arguments  + other_arguments
    print("executing \n ",cmd)
    output=subprocess.run(cmd,shell=True,capture_output=True, text=True).stdout
    print("finished with run")
    print(output)


    # compare results
    cmp=madjsoncompare(outputfile,referencefile)
    cmp.compare(["cis_excitations",0,"irrep"],1.0)
    cmp.compare(["cis_excitations",0,"omega"],1.e-4)
    cmp.compare(["cis_excitations",0,"oscillator_strength_length"],1.e-3)
    cmp.compare(["cis_excitations",0,"oscillator_strength_velocity"],1.e-3)
    print("final success: ",cmp.success)


    sys.exit(cmp.exitcode())