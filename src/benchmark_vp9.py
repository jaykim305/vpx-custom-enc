import os, argparse, re, time

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--vpxenc', type=str, default='../build/vpxenc')
    parser.add_argument('--input', type=str, required=True, help="must be yuv input")
    # parser.add_argument('--output', type=str, required=True, help="must be webm format")
    parser.add_argument('--bitrate', type=int, required=True)
    parser.add_argument('--logdir', type=str, required=True)
    args = parser.parse_args()

    speed = [5, 6, 7, 8]
    tile_columns = [0, 1, 2, 3, 4, 5]
    threads = [1, 2, 4, 8, 16, 32, 64]
    
    prefix = args.input.find('.yuv');
    output = f"{args.input[:prefix]}_{args.bitrate}.webm"

    # speed = [6]
    # tile_columns = [4]
    # threads = [16]
    # speed (5-8) / tile (0-5) (only on vp9) / threads (1, 2, 4, 8, 16, 32, 64) 
    # 4 * 6 * 7

    for s in speed:
        for t in tile_columns:
            for th in threads:

                print(f"===> benchmarking speed: {s}, tile: {t}, thread: {th}")

                cmd = f"{args.vpxenc} {args.input} -o {output} "
                cmd+= f"--codec=vp9 "
                cmd+= f"--i420 "
                cmd+= f"-w 3840 "
                cmd+= f"-h 2160 "
                cmd+= f"--end-usage=cbr "
                cmd+= f"--target-bitrate={args.bitrate} "
                cmd+= f"--fps=60000/1000 "
                cmd+= f"--kf-max-dist=90 "
                cmd+= f"--kf-min-dist=0 "
                cmd+= f"--passes=1 "
                cmd+= f"--pass=1 "
                cmd+= f"--rt "
                cmd+= f"--cpu-used={s} "
                cmd+= f"--tile-columns={t} "
                cmd+= f"--frame-parallel=1 "
                cmd+= f"--threads={th} "
                cmd+= f"--static-thresh=0 "
                cmd+= f"--max-intra-rate=300 "
                cmd+= f"--lag-in-frames=0 "
                cmd+= f"--min-q=4 "
                cmd+= f"--max-q=48 "
                cmd+= f"--row-mt=1 "
                cmd+= f"--error-resilient=1 "
                # cmd+= f"--psnr "
                cmd+= f"--limit=600 "
                # cmd+= f"--quiet "
                print(cmd)
                os.system(cmd)
                filesize = os.path.getsize(f"thread{th}_cpu{s}_tile{t}.log")
                if filesize == 0:
                    os.system(f"rm thread{th}_cpu{s}_tile{t}.log")
                os.system("pkill -f ../build/vpxenc")
                time.sleep(3)
                print("\n\n")                    
                # exit(1)
                


    summary = open("summary.log", "w")
    summary.write("speed tile thread fps\n")
    for s in speed:
        for t in tile_columns:
            for th in threads:
                log = f"thread{th}_cpu{s}_tile{t}.log"
                if os.path.isfile(log):
                    f = open(log, "r")
                    fps = re.findall("\d+.\d+ fps", f.readlines()[0])[0][:4]
                    print(fps)
                    summary.write(f"{s} {t} {th} {fps}\n")
    
    summary.close()
    
    os.makedirs(args.logdir, exist_ok=True)
    os.system(f"mv *.log {args.logdir}")
    