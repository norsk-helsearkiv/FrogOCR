import sys
from paddleocr import PaddleOCR

if __name__ == '__main__':
    if len(sys.argv) <= 1:
        print('Path to image or directory not specified.')
        exit(-1)
    image_path = sys.argv[1]
    paddle = PaddleOCR(use_angle_cls=True, lang='en')
    result = paddle.ocr(image_path, det=True, rec=False, cls=True)
    print('BEGIN PADDLE FROG')
    for index in range(len(result)):
        for line in result[index]:
            for coord in line:
                print(int(coord[0]), int(coord[1]), end=' ')
            print()
