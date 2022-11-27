import json
import glob
import cv2
import argparse
import os
from tqdm import tqdm
import sys

DEFAULT_IMPORT_DIR = "build/data/export"
DEFAULT_EXPORT_VIDEO = "build/data/export.mp4"
FPS = 30 # Number of frames per second in the generated video
BOX_COLOR = (0,255,0) # Green (BGR format, not RGB)
BOX_THICKNESS = 8

# Parse file arguments
parser = argparse.ArgumentParser()
parser.add_argument("--import_directory", help="The path to the folder containing the images", default=DEFAULT_IMPORT_DIR, nargs="?")
parser.add_argument("--export_video", help="The path of the generated MP4 video file as `build/export/export.mp4`", default=DEFAULT_EXPORT_VIDEO, nargs="?")

args = parser.parse_args()
import_dir_path = args.import_directory
export_video_path = args.export_video


# Find all images
image_list = glob.glob(os.path.join(import_dir_path, "input-*.png"))
image_list.sort()
print(f"{len(image_list)} images found !")

# Read JSON in STD IN
stdin_content = sys.stdin.read()
# Convert STDIN string to Dict
box_dict_2 = json.loads(stdin_content)
box_dict = {k.split('/')[-1]: v for k,v in box_dict_2.items()} 


def coord_matrix_to_opencv(x, y):
    return (x, y)


# Get shape of the image to determine the shape of the video
image = cv2.imread(image_list[0])
height, width, layers = image.shape

video_writer = cv2.VideoWriter(export_video_path, cv2.VideoWriter_fourcc(*'MP4V'), FPS, (width,height))

for image_path in tqdm(image_list):
    image_filename = os.path.basename(image_path)
    image_box_list = box_dict.get(image_filename, [])
    
    image = cv2.imread(image_path)

    # Draw each box on the current image
    for x, y, width, height in image_box_list:
        opencv_x, opencv_y = coord_matrix_to_opencv(x, y)
        start_point = (opencv_x, opencv_y)
        end_point = (opencv_x+width, opencv_y+height)
        image = cv2.rectangle(image, start_point, end_point, BOX_COLOR, BOX_THICKNESS)
    
    video_writer.write(image)

print("Generating video...")
video_writer.release()
print("Done.")
