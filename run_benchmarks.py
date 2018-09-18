import argparse, datetime, contextlib, json, multiprocessing, shlex, socket, subprocess, sys, time
from pathlib import Path
from shutil import copy

def launchSingleRun(cmd, outfile = None):
    ostream = open(outfile, "a") if outfile else sys.stdout
    with contextlib.redirect_stdout(ostream):
        cp = subprocess.run(cmd, stdout = sys.stdout, stderr = subprocess.STDOUT, check = True)
    

def launchRun(cmdA, cmdB, outfileA = None, outfileB = None):
    pA = multiprocessing.Process(target=launchSingleRun, daemon=True, args=(cmdA, outfileA))
    pB = multiprocessing.Process(target=launchSingleRun, daemon=True, args=(cmdB, outfileB))
    pA.start(); pB.start()
    pA.join();  pB.join()
    if (pA.exitcode != 0) or (pB.exitcode != 0):
        raise Exception



def split_file(inputfile, lines1, lines2, output1, output2):
    """ Split inputfile in two files, each containing linesN lines. Used for MPI machine files."""
    with open(inputfile) as f:
        lines = f.readlines()

    with open(output1, "w") as f:
        for i in range(0, lines1):
            f.write(lines[i])

    with open(output2, "w") as f:
        for i in range(lines1, lines1 + lines2):
            f.write(lines[i])


def generate_test_sizes(mpisize, platform):
    node_numbers = [1, 2, 3, 4, 6, 8, 10, 12, 14, 16, 18, 20, 24, 28, 32, 40, 48, 56, 64, 72, 80, 88, 96,
                    104, 112, 128,  144, 160, 176, 192, 208, 224, 240, 256, 272, 288, 304]
    if platform == "hazelhen":
        # Hazelhen node size is 24, only one mpi job per node.
        sizes = [24*i for i in node_numbers]
    elif platform == "supermuc":
        sizes = [28*i for i in node_numbers]
    else:
        sizes = range(2, mpisize+1)

    return [i for i in sizes if i <= mpisize]

def get_mpi_cmd(platform):
    if platform == "hazelhen":
        return "aprun -p fl_domain"
    elif platform == "supermuc":
        return "mpiexec"
    elif platform == "mpich-opt":
        return "/opt/mpich/bin/mpiexec --prepend-rank"
    elif platform == "mpich":
        return "mpiexec.mpich"
    else:
        return "mpiexec"


def get_machine_file(platform, size, inputfile):
    if platform == "supermuc":
        split_file(inputfile, size, size, "mfile.A", "mfile.B")
        return "-f mfile.A", "-f mfile.B"
    else:
        return ["", ""]
    
def removeEventFiles(participant):
    p = Path(participant)
    try:
        Path("%s-events.log" % participant).unlink()
        Path("%s-eventTimings.log" % participant).unlink()
    except FileNotFoundError:
        pass



def doScaling(name, ranks, peers, commTypes, debug):
    removeEventFiles("A")
    removeEventFiles("B")

    file_info = { "date" : datetime.datetime.now().strftime("%Y-%m-%d_%H:%M:%S"),
                  "name" : name}
    
    file_pattern = "{name}-{date}-{participant}.{suffix}"
    for rank, peer, commType in zip(ranks, peers, commTypes):
        cmd = "{mpi} -n {size} {machinefile} ./mpiports --peers {peers} --commType {comm} --rounds 1000 --participant {participant} {debug} --runName {runName}"
        cmdA = cmd.format(
            mpi = get_mpi_cmd(args.platform),
            size = rank,
            machinefile = get_machine_file(args.platform, rank, args.mfile)[0],
            peers = peer,
            comm = commType,
            participant = "A",
            debug = "--debug" if debug else "",
            runName = commType)
        cmdB = cmd.format(
            mpi = get_mpi_cmd(args.platform),
            size = rank,
            machinefile = get_machine_file(args.platform, rank, args.mfile)[1],
            peers = peer,
            comm = commType,
            participant = "B",
            debug = "--debug" if debug else "",
            runName = commType)

        print("Running on ranks = {}".format(rank))
        print(cmdA)
        print(cmdB)
        launchRun(shlex.split(cmdA), shlex.split(cmdB),
                  file_pattern.format(suffix = "out", participant = "A", **file_info),
                  file_pattern.format(suffix = "out", participant = "B", **file_info))                  

    copy("A-events.log", file_pattern.format(suffix = "events", participant = "A", **file_info))
    copy("A-eventTimings.log", file_pattern.format(suffix= "timings", participant = "A", **file_info))
    copy("B-events.log", file_pattern.format(suffix = "events", participant = "B", **file_info))
    copy("B-eventTimings.log", file_pattern.format(suffix= "timings", participant = "B", **file_info))

    with open("{name}-{date}.meta".format(**file_info), "w") as f:
        json.dump({"name"  : name,
                   "date" : file_info["date"],
                   "host" : socket.getfqdn(),
                   "ranks" : ranks,
                   "peers" : peers,
                   "commTypes" : commTypes},
                  f, indent = 4)
                



parser = argparse.ArgumentParser()
parser.add_argument("--mfile")
parser.add_argument("--mpisize", help = "Maximum MPI size per participant", type = int, required = True)
parser.add_argument("--peers", type=float, required = True)
parser.add_argument("--commType", choices = ["single", "many"], required=True)
parser.add_argument("--platform", choices = ["supermuc", "hazelhen", "mpich-opt", "mpich", "none"], default = "none")
parser.add_argument("--debug", action = "store_true", default = False)

args = parser.parse_args()

sizes = generate_test_sizes(args.mpisize, args.platform)

for i in range(5):
    doScaling(name = "benchmark_mpiports",
              ranks = sizes,
              peers = [args.peers] * len(sizes),
              commTypes = [args.commType] * len(sizes),
              debug = args.debug)
              
