# Molnár Martin - W4RO4U
# Valós idejű gyomfelismerés és számlálás neurális háló segítségével beágyazott Linux rendszeren

# Szükséges rendszer követelmények:
  - GCC 10.0+ (13.2.1 használtam)
  - OpenCV 4.5+ (4.8.1 használtam)

# Futtatás: 
  Linuxon:<br>
  ```bash
g++ -std=c++14 cpp/yolo.cpp cpp/log.cpp -o xy `pkg-config --cflags --libs opencv4`
./xy <input_video_filename> <output_video_filename>
```

vagy ha RTX videókártya van a számítógépben:

```bash
./xy <input_video_filename> <output_video_filename> cuda
```

# Működés:
 - A program elindítja a GPS-t(ha van), betölti a neurális hálót és megnyitja a videófájlt.
 - Létrehozz egy log.csv fájlt amiben később menti majd, a GPS szerinti gyomok számát.
 - Framenként keresi a képeken a gyomokat.
 - Ha van gyom akkor elmenti a log.csv-be, és készít egy másolatot a videóról amit elment.

# Néhány információ:
  A GPS szálkezelve van, és mutex-el védve, hogy ne crasheljen el ha a másik szálon éppen olvasnánk miközben a másik szál írja.

# Dimenziók és Sorok
  Netron alkalmazás segítségével meglehet nyitni a betanított neurális hálózatot és meglehet nézni az outputot:<br>
  A képen látszik, hogy dimenzió 25200 és 6 sor van.<br>
  
  ![neural_image](https://github.com/MolnarM22/szakdolgozat/blob/master/yolov5.png)

# Doxygen
  Doxygennel le generáltam hozzá némi dokumentációt.

# Felhasznált források:
  https://github.com/ultralytics/yolov5<br>
  https://universe.roboflow.com/td/g3_weeds<br>
  https://universe.roboflow.com/garbage-time/weedbot-luupi<br>
  https://universe.roboflow.com/weed-detector-wcwse/weed-detection-ha0nz<br>
  https://universe.roboflow.com/elf-lh29c/weeddetectorv1<br>



