import os, glob, argparse


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--input_dir', type=str, default='~/dataset-all')
    args = parser.parse_args()

    type = 'sr'

    png_dir = f'{args.input_dir}/{type}'
    yuv_dir = f'{args.input_dir}/{type}_yuv'

    pngs = glob.glob(f"{png_dir}/*.png")
    pngs.sort()

    # make png to yuv
    os.makedirs(yuv_dir, exist_ok=True)
    for idx, png in enumerate(pngs, 1):
        cmd = f"ffmpeg -y -loglevel error -i {png} -pix_fmt yuv420p {yuv_dir}/{idx}.yuv"
        print(f"[{idx}/{len(pngs)}] : {cmd}")
        os.system(cmd)

    # concat yuvs to single yuv
    for idx, png in enumerate(pngs, 1):
        cmd = f"cat {yuv_dir}/{idx}.yuv >> {args.input_dir}/{type}.yuv"
        print(f"[{idx}/{len(pngs)}] : {cmd}")
        os.system(cmd)
        
    print(f"==>YUV images saved to {yuv_dir} ... ")
    print(f"==>YUV file saved as {args.input_dir}/{type}.yuv ... ")