import argparse
import time
from pathlib import Path
import sys
import os

import cv2
import torch
import torch.backends.cudnn as cudnn
from numpy import random

from models.experimental import attempt_load
from utils.datasets import LoadStreams, LoadImages
from utils.general import check_img_size, check_requirements, check_imshow, non_max_suppression, apply_classifier, \
    scale_coords, xyxy2xywh, strip_optimizer, set_logging, increment_path
from utils.plots import plot_one_box
from utils.torch_utils import select_device, load_classifier, time_synchronized


def detect(save_img=False):
    # 解析参数
    source, weights, view_img, save_txt, imgsz = opt.source, opt.weights, opt.view_img, opt.save_txt, opt.img_size
    save_img = not opt.nosave and not source.endswith('.txt')  # 保存推理图片
    webcam = source.isnumeric() or source.endswith('.txt') or source.lower().startswith(
        ('rtsp://', 'rtmp://', 'http://', 'https://'))

    # 检查权重文件是否存在
    if not os.path.exists(weights[0] if isinstance(weights, list) else weights):
        print(f"错误: 权重文件 '{weights}' 不存在!")
        print("请确保以下文件存在:")
        print("1. weights/last.pt (默认)")
        print("2. weights/best.pt")
        print("3. 或者通过命令行指定: python detect.py --weights 你的权重文件.pt")
        sys.exit(1)

    # 检查源文件/目录是否存在
    if not source.isnumeric() and not source.startswith(('rtsp://', 'rtmp://', 'http://', 'https://')):
        if not os.path.exists(source):
            print(f"错误: 源路径 '{source}' 不存在!")
            print("请指定有效的图片/视频路径，或者使用 0 表示摄像头")
            sys.exit(1)

    # 创建输出目录
    save_dir = Path(increment_path(Path(opt.project) / opt.name, exist_ok=opt.exist_ok))
    (save_dir / 'labels' if save_txt else save_dir).mkdir(parents=True, exist_ok=True)

    # 初始化
    set_logging()
    device = select_device(opt.device)
    half = device.type != 'cpu'  # 只在CUDA上支持半精度

    # 加载模型
    print(f"加载模型: {weights}")
    model = attempt_load(weights, map_location=device)
    stride = int(model.stride.max())
    imgsz = check_img_size(imgsz, s=stride)

    if half:
        model.half()  # 转换为FP16

    # 第二级分类器（如果需要）
    classify = False
    if classify:
        modelc = load_classifier(name='resnet50', n=2)
        modelc.load_state_dict(torch.load('weights/classifier.pt', map_location=device)['model']).to(device).eval()

    # 设置数据加载器
    vid_path, vid_writer = None, None
    if webcam:
        view_img = check_imshow()
        cudnn.benchmark = True
        dataset = LoadStreams(source, img_size=imgsz, stride=stride)
    else:
        dataset = LoadImages(source, img_size=imgsz, stride=stride)

    # 获取类别名称和颜色
    names = model.module.names if hasattr(model, 'module') else model.names
    colors = [[random.randint(0, 255) for _ in range(3)] for _ in names]

    print(f"检测类别: {names}")
    print(f"输入源: {source}")
    print(f"设备: {device}")
    print(f"图片大小: {imgsz}")

    # 初始化FPS计算变量
    fps = 0
    frame_count = 0
    start_time = time.time()
    fps_window = 10  # 计算FPS的窗口大小（帧数）

    # 运行推理
    if device.type != 'cpu':
        model(torch.zeros(1, 3, imgsz, imgsz).to(device).type_as(next(model.parameters())))  # 预热
    t0 = time.time()

    for path, img, im0s, vid_cap in dataset:
        img = torch.from_numpy(img).to(device)
        img = img.half() if half else img.float()
        img /= 255.0
        if img.ndimension() == 3:
            img = img.unsqueeze(0)

        # 推理
        t1 = time_synchronized()
        pred = model(img, augment=opt.augment)[0]

        # NMS
        pred = non_max_suppression(pred, opt.conf_thres, opt.iou_thres, classes=opt.classes, agnostic=opt.agnostic_nms)
        t2 = time_synchronized()

        # 计算FPS
        inference_time = t2 - t1
        if inference_time > 0:
            current_fps = 1.0 / inference_time
        else:
            current_fps = 0

        # 更新平均FPS
        frame_count += 1
        if frame_count <= fps_window:
            fps = (fps * (frame_count - 1) + current_fps) / frame_count
        else:
            fps = fps * 0.9 + current_fps * 0.1  # 指数平滑

        # 应用分类器
        if classify:
            pred = apply_classifier(pred, modelc, img, im0s)

        # 处理检测结果
        for i, det in enumerate(pred):
            if webcam:
                p, s, im0, frame = path[i], '%g: ' % i, im0s[i].copy(), dataset.count
            else:
                p, s, im0, frame = path, '', im0s, getattr(dataset, 'frame', 0)

            p = Path(p)
            save_path = str(save_dir / p.name)
            txt_path = str(save_dir / 'labels' / p.stem) + ('' if dataset.mode == 'image' else f'_{frame}')
            s += '%gx%g ' % img.shape[2:]
            gn = torch.tensor(im0.shape)[[1, 0, 1, 0]]

            if len(det):
                # 将边界框从img_size缩放到im0大小
                det[:, :4] = scale_coords(img.shape[2:], det[:, :4], im0.shape).round()

                # 打印结果
                for c in det[:, -1].unique():
                    n = (det[:, -1] == c).sum()
                    s += f"{n} {names[int(c)]}{'s' * (n > 1)}, "  # 注意：这里有个变量名错误，应该是 names[int(c)]

                # 写入结果
                for *xyxy, conf, cls in reversed(det):
                    if save_txt:
                        xywh = (xyxy2xywh(torch.tensor(xyxy).view(1, 4)) / gn).view(-1).tolist()
                        line = (cls, *xywh, conf) if opt.save_conf else (cls, *xywh)
                        with open(txt_path + '.txt', 'a') as f:
                            f.write(('%g ' * len(line)).rstrip() % line + '\n')

                    if save_img or view_img:
                        label = f'{names[int(cls)]} {conf:.2f}'
                        plot_one_box(xyxy, im0, label=label, color=colors[int(cls)], line_thickness=opt.line_thickness)

            # 在图像左上角显示FPS
            if view_img:
                # 显示FPS
                fps_text = f'FPS: {fps:.1f}'
                fps_text2 = f'Inference: {inference_time * 1000:.1f}ms'

                # 设置文本样式
                font = cv2.FONT_HERSHEY_SIMPLEX
                font_scale = 0.6
                thickness = 2

                # 计算文本大小
                (fps_width, fps_height), baseline = cv2.getTextSize(fps_text, font, font_scale, thickness)
                (inf_width, inf_height), baseline = cv2.getTextSize(fps_text2, font, font_scale, thickness)

                # 在图像左上角添加黑色半透明背景
                overlay = im0.copy()
                cv2.rectangle(overlay, (10, 10), (max(fps_width, inf_width) + 30, 70), (0, 0, 0), -1)
                im0 = cv2.addWeighted(overlay, 0.6, im0, 0.4, 0)

                # 绘制FPS文本
                cv2.putText(im0, fps_text, (20, 40), font, font_scale, (0, 255, 0), thickness)
                cv2.putText(im0, fps_text2, (20, 70), font, font_scale, (255, 255, 0), thickness)

                # 显示帧号（如果是视频）
                if dataset.mode != 'image':
                    frame_text = f'Frame: {frame}'
                    cv2.putText(im0, frame_text, (20, 100), font, font_scale, (0, 200, 255), thickness)

            # 打印推理时间
            print(f'{s}完成. 推理: {inference_time * 1000:.1f}ms, FPS: {current_fps:.1f}')

            # 显示结果
            if view_img:
                cv2.imshow(str(p), im0)
                if cv2.waitKey(1) & 0xFF == ord('q'):  # 按q退出
                    raise StopIteration

            # 保存结果
            if save_img:
                if dataset.mode == 'image':
                    cv2.imwrite(save_path, im0)
                else:
                    if vid_path != save_path:
                        vid_path = save_path
                        if isinstance(vid_writer, cv2.VideoWriter):
                            vid_writer.release()
                        if vid_cap:
                            fps_save = vid_cap.get(cv2.CAP_PROP_FPS)
                            w = int(vid_cap.get(cv2.CAP_PROP_FRAME_WIDTH))
                            h = int(vid_cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
                        else:
                            fps_save, w, h = 30, im0.shape[1], im0.shape[0]
                            save_path += '.mp4'
                        vid_writer = cv2.VideoWriter(save_path, cv2.VideoWriter_fourcc(*'mp4v'), fps_save, (w, h))
                    vid_writer.write(im0)

    if save_txt or save_img:
        s = f"\n{len(list(save_dir.glob('labels/*.txt')))} labels saved to {save_dir / 'labels'}" if save_txt else ''
        print(f"结果保存到: {save_dir}{s}")

    # 打印平均FPS
    total_time = time.time() - t0
    avg_fps = frame_count / total_time
    print(f'平均FPS: {avg_fps:.2f}')
    print(f'全部完成. ({total_time:.3f}s)')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='YOLOv5-Lite 目标检测脚本')

    # 主要参数
    parser.add_argument('--weights', nargs='+', type=str, default='weights/last.pt', help='模型权重路径')
    parser.add_argument('--source', type=str, default=r'C:\Users\20466\Desktop\video4.mp4',
                        help='source')  # file/folder, 0 for webcam
    parser.add_argument('--img-size', type=int, default=640, help='推理图片大小 (像素)')

    # 检测参数
    parser.add_argument('--conf-thres', type=float, default=0.45, help='置信度阈值')
    parser.add_argument('--iou-thres', type=float, default=0.5, help='NMS的IOU阈值')
    parser.add_argument('--classes', nargs='+', type=int, help='按类别过滤: --class 0, 或 --class 0 2 3')
    parser.add_argument('--agnostic-nms', action='store_true', help='类别无关的NMS')
    parser.add_argument('--augment', action='store_true', help='增强推理')

    # 输出参数
    parser.add_argument('--view-img', action='store_true', default=True, help='显示结果')
    parser.add_argument('--save-txt', action='store_true', help='保存结果到*.txt文件')
    parser.add_argument('--save-conf', action='store_true', help='在--save-txt标签中保存置信度')
    parser.add_argument('--nosave', action='store_true', help='不保存图片/视频')
    parser.add_argument('--project', default='runs/detect', help='保存结果的路径')
    parser.add_argument('--name', default='exp', help='结果保存的文件夹名')
    parser.add_argument('--exist-ok', action='store_true', help='允许覆盖已存在的文件夹')

    # 其他参数
    parser.add_argument('--device', default='', help='CUDA设备: 0 或 0,1,2,3 或 cpu')
    parser.add_argument('--update', action='store_true', help='更新所有模型')
    parser.add_argument('--line-thickness', default=3, type=int, help='边界框线条粗细')

    # 新增FPS显示参数
    parser.add_argument('--show-fps', action='store_true', default=True, help='在画面上显示FPS')

    opt = parser.parse_args()

    # 打印参数
    print('=' * 50)
    print('YOLOv5-Lite 目标检测 (带FPS显示)')
    print('=' * 50)
    print(f'权重文件: {opt.weights}')
    print(f'输入源: {opt.source}')
    print(f'图片大小: {opt.img_size}')
    print(f'置信度阈值: {opt.conf_thres}')
    print(f'IOU阈值: {opt.iou_thres}')
    print('=' * 50)

    # 检查依赖
    check_requirements(exclude=('pycocotools', 'thop'))

    with torch.no_grad():
        if opt.update:
            for opt.weights in ['yolov5s.pt', 'yolov5m.pt', 'yolov5l.pt', 'yolov5x.pt']:
                detect()
                strip_optimizer(opt.weights)
        else:
            detect()