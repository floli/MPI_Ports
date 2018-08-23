""" Plots the results and creates a CSV for pgfplots. """

import argparse, csv, sys
import matplotlib.pyplot as plt
from EventTimings.EventTimings import getDataFrame

from ipdb import set_trace

parser = argparse.ArgumentParser()
parser.add_argument('--file', help = "Timings file")

if len(sys.argv) < 2:
    parser.print_help()
    sys.exit(1)

args = parser.parse_args()

df = getDataFrame(args.file)
df.reset_index(inplace = True)

sizes = []
for d in df.Timestamp.unique():
    sizes.append(df[df.Timestamp == d].Rank.max())

df = df[df.Rank == 0]

values = {
    "Publish" : [],
    "Connect" : [],
    "Data Send/Recv" : []}


for row in df.itertuples():
    size = df[df.Timestamp == row.Timestamp].Rank.max()
    if row.Name in values:
        values[row.Name].append(row.Total)


with open(args.file + ".csv", 'w', newline='') as csvfile:
    writer = csv.writer(csvfile)
    writer.writerow(["Size"] + list(values))
    for row in zip(sizes, *values.values()):
        writer.writerow(row)
    
for v in values:
    plt.plot(sizes, values[v], "d-", label = v)


plt.grid()
plt.title(args.file)
plt.xlabel("Ranks per participant")
plt.ylabel("Time [ms]")
plt.legend()
plt.show()
