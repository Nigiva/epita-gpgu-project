# GPGPU Project: object detection

## A) Goal of the project
The goal of the project is to implement a simple object detector in CUDA.

## B) Our group

| Name             | Email (@epita.fr)         | Github account |
| ---------------- | ------------------------- | -------------- |
| Corentin Duchêne | corentin.duchene          | `Nigiva`       |
| Théo Perinet     | theo.perinet              | `TheoPeri`     |
| Hao Ye           | hao.ye                    | `Enjoyshi`     |
| Arthur Fan       | arthur.fan                | `arthur942`    |

## C) How to run the project
To run the project, it is necessary to have installed :
* `Cmake`
* CUDA
* Python 3.8.12
* `python3-pip`
* `virtualenv`

We use [Poetry](https://python-poetry.org/) and [Pyenv](https://github.com/pyenv/pyenv) to generate the python virtual environment but this is not necessary to run the project (it is just recommended).

Run the following commands to import and build the project :
* Git clone the project
* Run `python -m venv .venv` in the project folder. Be careful to choose the right version of python, for example `python3.8 -m venv .venv`
* Run `source ./.venv/bin/activate` to activate the virtual environment
* Run `pip install -r requirements.txt`
* Run `cmake ..` in the `build/` folder (`cd build/`)
* Run `make all -j` in the `build/` folder


ℹ️ **Note :** To build the project in debug mode, you should use...
```
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

ℹ️ **Note 2 :** When building the project, `Cmake` takes care of downloading an example video `Aled.mp4` by itself into the `build/data` folder.

## D) How to use the project ?
### D.1) To convert a video to an image folder
To convert a video to an image folder, you can use [`convert_mp4_to_png.py`](convert_mp4_to_png.py).

By simply running 
```
python convert_mp4_to_png.py
```
the video `build/data/Aled.mp4` will be converted into a folder of images contained in `build/data/export/`. This folder is deleted on each re-generation.

If you want to choose the video to be converted or the destination folder for the images, you can read the documentation by running :
```
python convert_mp4_to_png.py -h
```

### D.2) To run object detection on a series of images
```
./build/detect build/data/export/reference.png $(ls build/data/export/input-*.png)
```

### D.3) To display the boxes in a video
It is possible to generate a video with the detected objects framed by boxes from the JSON given in STDIN and an image folder.

We provide a JSON example `boxes_stdout_example.json` to debug our python program.
So you can see the result by running the following commands:
* `python convert_mp4_to_png.py` to generate the image folder from the video supplied by default with the project
* `cat boxes_stdout_example.json | python generate_video.py`

Normally, you should see :
* 2 green boxes
   * one starts at the top left of the image 
  * the other also starts at the top left but shifted 100px to the right and 200px to the bottom.
* The top left box already has a height of 100px at the beginning and increases in height and width linearly.
* The other box has a size of zero at the start but grows faster in width than in height.
  
To generate the video normally from the STDOUT of the binary, you can therefore run the following command :
```
./build/detect build/data/export/reference.png $(ls build/data/export/input-*.png) | python generate_video.py
```

If you want to choose the image folder or the path of the exported video, you can read the documentation by running :
```
python generate_video.py -h
```

### D.4) To run the benchmark
```
./build/bench build/data/export/reference.png $(ls build/data/export/input-*.png)
```
