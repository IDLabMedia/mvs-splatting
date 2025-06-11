# Program that reads in and print the log generated when running train_benchmark.py

import sys
import os
import numpy as np
import matplotlib.pyplot as plt
import cv2
from datetime import datetime

if len(sys.argv) != 2:
    print("Unexpected command line args.")
    print("Usage:\n  python read_log.py <path/to/log.txt>")
    exit(0)
    
log_path = sys.argv[1]

def read_log_txt(log_path):
    if not os.path.isfile(log_path):
        print("Error: could not find file:", log_path)
        return False, None
        
    with open(log_path, "r") as file:
        txt = file.readlines()

    # First 2 lines: test an training camera names
    assert(txt[0].startswith("test cams"))
    assert(txt[1].startswith("training cams"))
    test_cams = sorted(txt[0][11:].strip().split(", "))
    train_cams = sorted(txt[1][15:].strip().split(", "))
    test_cams = [c for c in test_cams if c != '']
    train_cams = [c for c in train_cams if c != '']
    names = test_cams + train_cams
    names = sorted(names)
    
    # make a list of bools showing whether or not a cam is for training
    cam_is_training = [(t, False) for t in test_cams] + [(t, True) for t in train_cams]
    cam_is_training.sort(key=lambda tup: tup[0])
    is_training = [t[1] for t in cam_is_training]

    # extract the psnr for each iteration
    iterations = []
    psnrs = []
    ssims = []
    lpips = []
    for line in txt[2::4]:
        values = line.split(", ")
        iterations.append(int(values[0]))
        psnrs.append([ float(x) for x in values[1:] ])
    for line in txt[3::4]:
        values = line.split(", ")
        ssims.append([ float(x) for x in values[1:] ])
    for line in txt[4::4]:
        values = line.split(", ")
        lpips.append([ float(x) for x in values[1:] ])

    iterations = np.array(iterations)
    is_training = np.array(is_training)
    psnrs = np.array(psnrs)
    ssims = np.array(ssims)
    lpips = np.array(lpips)
    
    if psnrs.shape[0] == 0:
        print("Error: unexpected psnrs.shape:", psnrs.shape)
        return False, None

    # extract the L1 loss, time since start, #gaussians, # memory usage (bytes)
    it_info = {}
    it_info_keys = ["L1", "time", "nr_gaussians", "mem_usage"]
    for k in it_info_keys:
        it_info[k] = []

    for line in txt[5::4]:
        values = line.split(", ")
        for i,k in enumerate(it_info_keys):
            it_info[k].append(float(values[i+1]) if k != "nr_gaussians" else int(values[i+1]))

    if psnrs.shape[1] == is_training.shape[0]:
        # save in a dictionary to compare to other datasets
        log = {
            "iterations": iterations,    # shape = (#iterations,)
            "names": names,              # shape = (#cameras,)
            "is_training": is_training,  # shape = (#cameras,)
            "all_psnrs": psnrs,          # shape = (#iterations, #cameras)
            "all_ssims": ssims,          # shape = (#iterations, #cameras)
            "all_lpips": lpips,          # shape = (#iterations, #cameras)
            "test_psnrs": psnrs[:,~is_training],   # shape = (#iterations, #test cameras)
            "test_ssims": ssims[:,~is_training],   # shape = (#iterations, #test cameras)
            "test_lpips": lpips[:,~is_training],   # shape = (#iterations, #test cameras)
            "train_psnrs": psnrs[:,is_training],   # shape = (#iterations, #train cameras)
            "train_ssims": ssims[:,is_training],   # shape = (#iterations, #train cameras)
            "train_lpips": lpips[:,is_training],   # shape = (#iterations, #train cameras)
            "it_info": it_info,          # dictionary
        }
    elif psnrs.shape[1] == np.sum(is_training == False):
        # save in a dictionary to compare to other datasets
        log = {
            "iterations": iterations,    # shape = (#iterations,)
            "names": names,              # shape = (#cameras,)
            "test_psnrs": psnrs,         # shape = (#iterations, #test cameras)
            "test_ssims": ssims,         # shape = (#iterations, #test cameras)
            "test_lpips": lpips,         # shape = (#iterations, #test cameras)
            "it_info": it_info,          # dictionary
        }
    else:
        print("Error: psnrs.shape[1] should be equal to is_training.shape[0] (in not --eval mode), or equal to np.sum(is_training == False) (in --eval mode)")
        return False, None
    return True, log

# Read in the log file
ok, log = read_log_txt(log_path)
if not ok:
    print("Reading log failed.")
    exit(0)
    
# Print the resulting log dictionary
print("The variable \'log\' is now a dictionary:")
print("{")
for key, value in log.items():
    message = ""
    if type(value) == np.ndarray:
        message = "numpy array of shape " + str(value.shape)
    elif type(value) == list:
        message = "list of length " + str(len(value))
    elif type(value) == dict:
        message = "dictionary with keys " + str(value.keys())
    print("\t\"{}\": {}".format(key, message))
print("}")

print("Feel free to add your own code that further processes this dictionary.")