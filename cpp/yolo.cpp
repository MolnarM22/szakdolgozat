#include <fstream>
#include <iostream>
#include <fstream>
#include <thread>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include "gps.hpp"

using namespace std;
using namespace cv;

bool gpsIsConnected; /*!< Ha USB csatlakoztatva akkor true. */


const float INPUT_WIDTH = 640.0; /*!< Kép csatolva a GitHub repoba */
const float INPUT_HEIGHT = 640.0; /*!< Kép csatolva a GitHub repoba */
const float SCORE_THRESHOLD = 0.05;
const float NMS_THRESHOLD = 0.1;
const float CONFIDENCE_THRESHOLD = 0.06;

/**
 *  Detektált objektumok tárolása.
 *
 */
 struct Detection
{
    int class_id; /*!< Detektált class ID-ja */
    float confidence; /*!< Detektált objektum pontszáma. */
    Rect box; /*!< Detektált objektum paraméterei: x,y,w,h*/
};

/**
 * Betölti a classokat, amiket detektálni tudunk.
 *
 * @return A classok neveit tartalmazó string lista.
 */
vector<string> load_class_list()
{
    vector<string> class_list;
    ifstream ifs("config_files/v4.txt"); // Fájl megnyitása
    string line;
    while (getline(ifs, line)) // Soronként olvasás
    {
        class_list.push_back(line); // Sor hozzáadása a listához
    }
    return class_list;
}

/**
 * Betölti a neurális hálót.
 *
 * @param net A neurális háló objektuma.
 * @param is_cuda Ha igaz, akkor a CUDA magokat használja.
 */
void load_net(dnn::Net &net, bool is_cuda)
{
    auto result = dnn::readNet("config_files/v4.onnx");
    if (is_cuda)
    {
        cout << "Cuda magok használata\n";
        result.setPreferableBackend(dnn::DNN_BACKEND_CUDA);
        result.setPreferableTarget(dnn::DNN_TARGET_CUDA_FP16);
    }
    else
    {
        cout << "CPU használata\n";
        result.setPreferableBackend(dnn::DNN_BACKEND_OPENCV);
        result.setPreferableTarget(dnn::DNN_TARGET_CPU);
    }
    net = result;
}

const vector<Scalar> colors = {Scalar(255, 255, 0), Scalar(0, 255, 0), Scalar(0, 255, 255), Scalar(255, 0, 0)};

/**
 * Formázza az input képet a YOLOv5 modell számára.
 *
 * @param source Az eredeti kép.
 * @return A formázott kép.
 */
Mat format_yolov5(const Mat &source) {
    int col = source.cols;
    int row = source.rows;
    int _max = MAX(col, row);
    Mat result = Mat::zeros(_max, _max, CV_8UC3);
    source.copyTo(result(Rect(0, 0, col, row))); 
    return result;
}

/**
 * Objektumok detektálása egy képen.
 *
 * @param image A bemeneti kép.
 * @param net neurális háló
 * @param output detekt lista
 * @param className osztálynevek listája
 */
void detect(Mat &image, dnn::Net &net, vector<Detection> &output, const vector<string> &className) {
    Mat blob;

    auto input_image = format_yolov5(image); 
    dnn::blobFromImage(input_image, blob, 1./255., Size(INPUT_WIDTH, INPUT_HEIGHT), Scalar(), true, false); 
    net.setInput(blob); 
    vector<Mat> outputs;
    net.forward(outputs, net.getUnconnectedOutLayersNames()); 


    float x_factor = input_image.cols / INPUT_WIDTH;
    float y_factor = input_image.rows / INPUT_HEIGHT;
    
    float *data = (float *)outputs[0].data; 
  
    
    const int dimensions = 6; 
    const int rows = 25200; //A neurális hálóban lévő három concatból adódik össze 19200 + 4800 + 1200
    
    vector<int> class_ids;
    vector<float> confidences;
    vector<Rect> boxes;

    for (int i = 0; i < rows; ++i) {
        float confidence = data[4];
        cout << data << ": " << confidence << endl;
        if (confidence >= CONFIDENCE_THRESHOLD) {

            float * classes_scores = data + 5; 
            Mat scores(1, className.size(), CV_32FC1, classes_scores);
            Point class_id;
            double max_class_score;
            minMaxLoc(scores, 0, &max_class_score, 0, &class_id);
            if (max_class_score > SCORE_THRESHOLD) {

                confidences.push_back(confidence); 

                class_ids.push_back(class_id.x); 

                float x = data[0];
                float y = data[1];
                float w = data[2];
                float h = data[3];
                int left = int((x - 0.5 * w) * x_factor);
                int top = int((y - 0.5 * h) * y_factor);
                int width = int(w * x_factor);
                int height = int(h * y_factor);
                boxes.push_back(Rect(left, top, width, height)); 
            }

      
        }
        data +=6; //Következő memoria címre ugrunk

    }
    vector<int> nms_result;
    dnn::NMSBoxes(boxes, confidences, SCORE_THRESHOLD, NMS_THRESHOLD, nms_result); 
    for (int i = 0; i < nms_result.size(); i++) {
        int idx = nms_result[i];
        Detection result;
        result.class_id = class_ids[idx];
        result.confidence = confidences[idx];
        result.box = boxes[idx];
        output.push_back(result); 
    }

}

int main(int argc, char **argv)
{
    cout << CV_VERSION << endl;
    gpsIsConnected = gpsInit(); 
    if(gpsIsConnected){
      thread gpsThread(gpsCom); 
      gpsThread.detach();
    }
    
    vector<string> class_list = load_class_list(); 

    printf("Input video: %s\n ", argv[1]);
    printf("Output video: %s\n ", argv[2]);
    Mat frame;
    VideoCapture capture(argv[1]); 
    if (!capture.isOpened())
    {
        cerr << "Hiba a videó fájl megnyitása közben:\n Biztos létezik ilyen?\n Lehet rossz a kódolás \n";
        return -1;
    }

    bool is_cuda = argc > 1 && strcmp(argv[2], "cuda") == 0; 

    dnn::Net net;
    load_net(net, is_cuda); 
    capture.read(frame);
    auto start = chrono::high_resolution_clock::now(); 
    int frame_count = 0;
    float fps = -1;
    int total_frames = 0;
    int frame_width = 1080;
    int frame_height = 1920;
    
    VideoWriter video(argv[2], VideoWriter::fourcc('a','v','c','1'), 10, Size(frame_width,frame_height)); 
    
    ofstream outfile;
    outfile.open("log.csv", ios_base::app); 
    outfile << "Weed Count;" << "GPS Lat;" << "GPS Lon \n"; 
    
    while (true)
    {

        capture.read(frame); 
        if (frame.empty())
        {
            cout << "Vége a videónak\n";
            break;
        }

        vector<Detection> output;
        detect(frame, net, output, class_list); 
        
        frame_count++;
        total_frames++;

        int detections = output.size(); 
        
        for (int i = 0; i < detections; ++i)
        {

            auto detection = output[i];
            auto box = detection.box;
            auto classId = detection.class_id;
            const auto color = colors[classId % colors.size()]; 
            rectangle(frame, box, color, 3); 

            rectangle(frame, Point(box.x, box.y - 20), Point(box.x + box.width, box.y), color, FILLED); 
            putText(frame, class_list[classId].c_str(), Point(box.x, box.y - 5), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0)); 
        }

        if (frame_count >= 30)
        {

            auto end = chrono::high_resolution_clock::now(); 
            fps = frame_count * 1000.0 / chrono::duration_cast<chrono::milliseconds>(end - start).count(); 

            frame_count = 0;
            start = chrono::high_resolution_clock::now(); 
        }

        if (fps > 0)
        {

            ostringstream fps_label;
            fps_label << fixed << setprecision(2);
            fps_label << "FPS: " << fps;
            string fps_label_str = fps_label.str();

            ostringstream weed_sum_label;
            weed_sum_label << fixed << setprecision(2);
            weed_sum_label << "Detected Weed: " << detections;
            string weed_sum_label_str = weed_sum_label.str();
            
            if(gpsIsConnected){
                GPSDataClass gpsData =  returnGPSData();
                ostringstream gpsCor;
                gpsCor << fixed << setprecision(2);
                gpsCor << "GPS Coordinates  " << gpsData.lat << "  " <<gpsData.lon;
                string gpsCor_str = gpsCor.str();
                cout<< "GPS type: " << gpsData.type<<endl;
                cout<< "GPS time: " << gpsData.time<<endl;
                outfile << detections <<";"<< gpsData.lat <<";"<< gpsData.lon<<"\n"; 
                putText(frame, gpsCor_str.c_str(), Point(10, 105), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 2); 
            }
            
            putText(frame, weed_sum_label_str.c_str(), Point(10, 75), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 2); 
            putText(frame, fps_label_str.c_str(), Point(10, 25), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 2); 

        }
        int down_width = frame.size().width;
        int down_height = frame.size().height;
        if(frame.size().height > frame.size().width){
            down_width = 480;
            down_height = 720;
        } else {
            down_width = 720;
            down_height = 1280;
        }
        Mat resized_down;
        resize(frame, resized_down, Size(down_width, down_height), INTER_LINEAR); 
        
        video.write(frame); 
        imshow("output", resized_down);

        if (waitKey(1) != -1)
        {
            capture.release();
            cout << "befejezve \n";
            break;
        }
    }

    cout << "Az összes frame ami fel lett dolgozva: " << total_frames << "\n"; 
    video.release(); 
    outfile.close(); 
    
    return 0;
}

