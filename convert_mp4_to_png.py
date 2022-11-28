import cv2
import argparse
import shutil
import os
from tqdm import tqdm

DEFAULT_VIDEO = "build/data/Aled.mp4"
DEFAULT_EXPORT_DIR = "build/data/export"

# Parse file arguments
parser = argparse.ArgumentParser()
parser.add_argument("--video", help="Video path to convert from MP4 to PNG", default=DEFAULT_VIDEO, nargs="?")
parser.add_argument("--export_directory", help="The path of the folder that will contain the converted images", default=DEFAULT_EXPORT_DIR, nargs="?")

args = parser.parse_args()
video_path = args.video
export_path = args.export_directory

# Clean export path & create if it is necessary
if os.path.isdir(export_path):
    shutil.rmtree(export_path)

os.makedirs(export_path, exist_ok=True)

# Check that the video file exists
if not os.path.exists(video_path):
    raise IOError(f"This video does not exist : {video_path}")

# Convert MP4 to PNG
## reference.png input-0001.png input-0002.png input-0003.png
video_capture = cv2.VideoCapture(video_path)
max_count = int(video_capture.get(cv2.CAP_PROP_FRAME_COUNT))
progress = tqdm(total=max_count)

# Export the reference image
success, image = video_capture.read()
reference_path = os.path.join(export_path, "reference.png")
cv2.imwrite(reference_path, image)
progress.update(1)

success, image = video_capture.read()

count = 1

# Export each image of the video
while success:
    image_path = os.path.join(export_path, f"input-{count:04d}.png")
    cv2.imwrite(image_path, image)
    progress.update(1)

    success, image = video_capture.read()
    count += 1
