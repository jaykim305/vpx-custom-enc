import os, glob, math, json, argparse
from PIL import Image
import numpy as np


def get_psnr(pred, gt, max_value=1.0):
    pred = pred.astype(np.float32)
    gt = gt.astype(np.float32)
    mse = np.mean((pred-gt)**2)
    if mse == 0:
        return 100
    else:
        return 20*math.log10(max_value/math.sqrt(mse))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--input_dir', type=str, required=True)
    parser.add_argument('--type', type=str, required=True)
    parser.add_argument('--bitrate', type=int, required=True)
    args = parser.parse_args()

    prefix = f"sr_{args.bitrate}k_{args.type}"
    input = f'{args.input_dir}/{prefix}.webm'
    
    sr_dir = f'{args.input_dir}/{prefix}'
    hr_dir = f'{args.input_dir}/hr'
    os.makedirs(sr_dir, exist_ok=True)
    
    os.system(f"rm {sr_dir}/*")
    
    cmd = f"ffmpeg -y -i {input} {sr_dir}/%04d.png"
    print(cmd)
    os.system(cmd)

    hr_pngs = glob.glob(f"{hr_dir}/*.png")
    sr_pngs = os.listdir(sr_dir)
    
    # print(len(hr_pngs))
    # print(len(sr_pngs))
    # assert(len(hr_pngs) == len(sr_pngs))
    sr_pngs.sort()

    with open(f"{args.input_dir}/sr/quality.json") as f:
        quality = json.load(f)['sr']

    log = open(f"{args.input_dir}/{prefix}_quality.log", "w")
    log.write("frameIndex diff compressed raw\n")
    for idx, filename in enumerate(sr_pngs, 0):
        sr = Image.open(f"{sr_dir}/{filename}")
        sr = np.array(sr)
        hr = Image.open(f"{hr_dir}/{filename}")
        hr = np.array(hr)
        psnr = get_psnr(sr, hr, max_value=255.0)
        print(f"[{idx+1}/{len(sr_pngs)}]DIFF: {float(quality[idx])-psnr} compressed: {psnr}, image: {quality[idx]}")
        log.write(f"{idx+1} {float(quality[idx])-psnr} {psnr} {quality[idx]}\n")
    
    log.close()