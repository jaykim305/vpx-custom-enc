import os, glob

base_dir = '/home/jaykim305/dataset-all'
type = 'sr'

png_dir = f'{base_dir}/{type}'
yuv_dir = f'{base_dir}/{type}_yuv'

pngs = glob.glob(f"{png_dir}/*.png")
pngs.sort()

os.makedirs(yuv_dir, exist_ok=True)
for idx, png in enumerate(pngs, 1):
    cmd = f"ffmpeg -y -i {png} -pix_fmt yuv420p {yuv_dir}/{idx}.yuv"
    print(cmd)
    os.system(cmd)


for idx, png in enumerate(pngs, 1):
    cmd = f"cat {yuv_dir}/{idx}.yuv >> {base_dir}/{type}.yuv"
    print(cmd)
    os.system(cmd)