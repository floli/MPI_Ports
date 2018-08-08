import argparse, datetime, contextlib, json, multiprocessing, shlex, socket, subprocess, sys, time
from pathlib import Path
from shutil import copy


def launchSingleRun(cmd, outfile = None):
    outfile = None
    ostream = open(outfile, "a") if outfile else sys.stdout
    print(cmd)
    with contextlib.redirect_stdout(ostream):
        cp = subprocess.run(cmd, stdout = sys.stdout, stderr = subprocess.STDOUT, check = True)
    

def launchRun(cmdA, cmdB, outfileA = None, outfileB = None):
    pA = multiprocessing.Process(target=launchSingleRun, daemon=True, args=(cmdA, outfileA))
    pB = multiprocessing.Process(target=launchSingleRun, daemon=True, args=(cmdB, outfileB))
    pA.start(); time.sleep(1); pB.start()
    pA.join();  pB.join()
    if (pA.exitcode != 0) or (pB.exitcode != 0):
        raise Exception


def ccall(cmd,  **kwargs):
    """ Runs cmd in a shell, returns its return code. Raises exception on error """
    print(cmd)
    # return call(cmd, check = True, **kwargs)



def split_file(inputfile, lines1, lines2, output1, output2):
    with open(inputfile) as f:
        lines = f.readlines()

    with open(output1, "w") as f:
        for i in range(0, lines1):
            f.write(lines[i])

    with open(output2, "w") as f:
        for i in range(lines1, lines1 + lines2):
            f.write(lines[i])


def generate_test_sizes(mpisize, platform):
    node_numbers = [1, 2, 3, 4, 6, 8, 10, 12]
    if platform == "hazelhen":
        # Hazelhen node size is 24, only one mpi job per node.
        sizes = 24 * node_numbers
    elif platform == "supermuc":
        sizes = 28 * node_numbers
    else:
        sizes = 1 * node_numbers
        sizes = sizes[1:] # remove the first element, because -n 1 can lead to problems

    return [i for i in sizes if i < mpisize]

def get_mpi_cmd(platform):
    if platform == "hazelhen":
        return "aprun"
    elif platform == "supermuc":
        return "mpiexec"
    elif platform == "mpich-opt":
        return "/opt/mpich/bin/mpiexec --prepend-rank"
    elif platform == "mpich":
        return "mpiexec.mpich"
    else:
        return "mpiexec"

def removeEventFiles(participant):
    p = Path(participant)
    try:
        Path("Events-%s.log" % participant).unlink()
        Path("EventTimings-%s.log" % participant).unlink()
    except FileNotFoundError:
        pass



def doScaling(name, ranks, peers, commTypes, debug):
    removeEventFiles("A")
    removeEventFiles("B")

    file_info = { "date" : datetime.datetime.now().isoformat(),
                  "name" : name}
    
    file_pattern = "{name}-{date}-{participant}.{suffix}"
    
    for rank, peer, commType in zip(ranks, peers, commTypes):
        cmd = "{mpi} -n {size} ./mpiports --peers {peers} --commType {comm} --rounds 1000".format(
            mpi = get_mpi_cmd(args.platform),
            size = rank,
            peers = peer,
            comm = commType)
        if debug:
            cmd += " --debug"
        cmdA = cmd + " --participant=A"
        cmdB = cmd + " --participant=B"

        print("Running on ranks = {}".format(rank))
        print(cmdA)
        launchRun(shlex.split(cmdA), shlex.split(cmdB),
                  file_pattern.format(suffix = "out", participant = "A", **file_info),
                  file_pattern.format(suffix = "out", participant = "B", **file_info))                  


    copy("Events-A.log", file_pattern.format(suffix = "events", participant = "A", **file_info))
    copy("EventTimings-A.log", file_pattern.format(suffix= "timings", participant = "A", **file_info))
    copy("Events-B.log", file_pattern.format(suffix = "events", participant = "B", **file_info))
    copy("EventTimings-B.log", file_pattern.format(suffix= "timings", participant = "B", **file_info))

    with open("{name}-{date}.meta".format(**file_info), "w") as f:
        json.dump({"name"  : name,
                   "date" : file_info["date"],
                   "host" : socket.getfqdn(),
                   "ranks" : ranks,
                   "peers" : peers,
                   "commTypes" : commTypes},
                  f)
                



parser = argparse.ArgumentParser()
parser.add_argument("--mfile")
parser.add_argument("--mpisize", type = int, required = True)
parser.add_argument("--peers", type=float, required = True)
parser.add_argument("--commType", choices = ["single", "many"], required=True)
parser.add_argument("--platform", choices = ["supermuc", "hazelhen", "mpich-opt", "mpich", "none"], default = "none")
parser.add_argument("--debug", action = "store_true", default = False)

args = parser.parse_args()

sizes = generate_test_sizes(args.mpisize, args.platform)

doScaling(name = "benchmark_mpiports",
          ranks = sizes,
          peers = [args.peers] * len(sizes),
          commTypes = [args.commType] * len(sizes),
          debug = args.debug)
              
