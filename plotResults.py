import argparse, sys
import matplotlib.pyplot as plt
from EventTimings.EventTimings import getDataFrame

from ipdb import set_trace

parser = argparse.ArgumentParser()
parser.add_argument('--file')

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
    "Connect" : [],
    "Data Send/Recv" : []}


for row in df.itertuples():
    size = df[df.Timestamp == row.Timestamp].Rank.max()
    if row.Name in values:
        values[row.Name].append(row.Total)


for v in values:
    plt.plot(sizes, values[v], label = v)


plt.grid()
plt.xlabel("Ranks")
plt.ylabel("Time [ms]")
plt.legend()
plt.show()
