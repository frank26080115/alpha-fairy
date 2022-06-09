from PIL import Image
import os, glob

for i in glob.glob("*.png"):
    j = Image.open(i)
    nm, ext = os.path.splitext(i)
    savename = nm + ".jpg"
    if os.path.isfile(savename):
        os.remove(savename)
    j.convert("RGB").save(savename, quality=100, subsampling=0)
    print(savename)
