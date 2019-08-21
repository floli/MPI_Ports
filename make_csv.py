#!/usr/bin/env python3
""" Plots the results and creates a CSV for pgfplots. """

import argparse, csv, sys
import matplotlib.pyplot as plt
import pandas as pd

from ipdb import set_trace

def drop_and_mean(x):
    if len(x) >= 5:
        return x.mask((x.index == x.idxmax()) | (x.index == x.idxmin())).mean()
    else:
        print("Less than 5 values in series, only applying mean.")
        return x.mean()
    

def add_MPIsize(x):
    """ Adds the maximum Rank in x as new column MPIsize. """
    x["MPIsize"] = x["Rank"].max()+1
    return x


parser = argparse.ArgumentParser()
parser.add_argument('--files', help = "File names of log file", nargs = "+")
args = parser.parse_args()

dfs = [pd.read_csv(f, parse_dates = [0], comment = "#") for f in args.files]
df = pd.concat(dfs)
df = df.reset_index(drop = True)

df = df.groupby(["Timestamp"], as_index = False).apply(add_MPIsize)
df = df[df["Rank"] == 0]

df = df.groupby(["RunName", "Name", "MPIsize"], as_index = False).aggregate({"Total" : drop_and_mean})

for name, group in df.groupby("RunName"):
    pdf = group.pivot(index = "MPIsize", columns = "Name", values = "Total")
    pdf.to_csv(name + ".csv")
    pdf.plot(title = name, grid = True)
    plt.show()
