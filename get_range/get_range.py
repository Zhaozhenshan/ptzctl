#!/usr/bin/env python3

import math
import argparse


def is_empty_line(line) -> bool:
    '''检查csv文件中的line行是否为空

    :param:  line, 已被spilt(',')处理
    :return: True, 当且仅当列表里所有项都是空
    '''
    flag = True
    for x in line:
        if x != '':
            flag = False
            break
    return flag


def cal_distance(coord1, coord2) -> float:
    # 计算两点间距离

    x1, y1, x2, y2 = coord1[0], coord1[1], coord2[0], coord2[1]
    delta_x = x2 - x1
    delta_y = y2 - y1
    return math.sqrt(delta_x * delta_x + delta_y * delta_y)


def check_sensor(sensor):
    # 检查sensor是否为 radar 或 upstream

    return str(sensor).find('radar') != -1 or str(sensor).find('upstream') != -1


class DataAnalysis:
    def __init__(self, file_name) -> None:
        self.file_name = file_name

        self.distance = []  # 距离列表

        self.mean = 0.0  # 高斯分布均值
        self.sigma = 0.0  # 高斯分布方差

        self.left = 0.0  # 区间左值
        self.right = 0.0  # 区间右值
        self.accuracy = 0.0  # 准确率，有百分之多少'距离'在上面的区间里

        self.get_data()
        self.cal_property()

    def readcsv(self):
        ''' 根据file_name读取csv文件并进行预处理

        :param: file_name, csv文件名称
        :return: data, 去除结尾的\n, 每一行数据用spilt函数处理
        '''
        try:
            fd = open(self.file_name, 'r')
            data = fd.readlines()
            for i in range(len(data)):
                data[i] = data[i].replace('\n', '')
                data[i] = data[i].split(',')
            return data
        except Exception:
            print('读取文件失败')
            return []

    def data2group(self, csv_data) -> list:
        # 将csv中的数据分组
        data = [[]]
        for line in csv_data:
            if is_empty_line(line):
                data.append([])
                continue
            data[-1].append(line)
        return data

    def get_distance_list(self, data):
        ''' 分组处理, 返回distance列表

        :param: data, 分组后的数据
        :return: distance列表
        '''
        distance = []
        for k in range(len(data)):
            sensor = {}
            for row in data[k]:
                if row[1] == 'fusion':  # 如果是融合结果，直接处理
                    source = []
                    for i in range(24, len(row)-1, 2):
                        if(check_sensor(row[i])):
                            source.append(str(row[i]) + ',' + str(row[i+1]))
                    if len(source) >= 2 and source[0] in sensor.keys() and source[1] in sensor.keys():
                        distance.append(
                            cal_distance(sensor[source[0]], sensor[source[1]]))

                else:  # 如果是其他sensor获取的数据, 把坐标存到sensor里
                    utm_x, utm_y = row[5], row[6]
                    coord = []
                    coord.append(float(utm_x))
                    coord.append(float(utm_y))
                    sensor[str(row[1]) + ',' + str(row[3])] = coord

        self.distance = distance

    def cal_mean_sigma(self) -> tuple:
        # 根据高斯分布的均值及方差的最大似然估计, 计算这组数据的均值和标准
        n = len(self.distance)
        sum = 0.0
        square_sum = 0.0
        for x in self.distance:
            sum += x
            square_sum += x * x

        mean = sum / (n * 1.0)  # 均值
        var = square_sum / (n * 1.0) - mean * mean  # 方差 : 平方的期望 - 期望的平方

        sigma = math.sqrt(var)
        self.mean, self.sigma = mean, sigma

    def cal_range(self) -> tuple:
        # 根据高斯分布的 3 sigma 原则计算区间的边界
        left = self.mean - 3 * self.sigma
        right = self.mean + 3 * self.sigma
        self.left, self.right = max(left, 0), right

    def cal_accuracy(self) -> float:
        # 统计有多少个'距离'在区间上
        n = len(self.distance)
        sum = 0
        for x in self.distance:
            if x >= self.left and x <= self.right:
                sum += 1
        self.accuracy = (sum * 1.0) / n

    def get_data(self):
        # 读取csv数据, 把符合条件的融合结果中的传感器距离存到列表中, 比如某一条融合结果是由 radar 和 upstream检测到的，distance即两者的距离
        data = self.readcsv()
        data = self.data2group(data)
        self.get_distance_list(data)

    def cal_property(self):
        # 根据distance列表计算一些属性,比如均值方差,边界值,准确率
        self.cal_mean_sigma()
        self.cal_range()
        self.cal_accuracy()

    def __str__(self) -> str:
        return 'file:' + str(self.file_name) + '\trange:\t[' + str(self.left) + ',' + str(self.right) + ']' \
            + '\taccuracy: ' + str(self.accuracy)


def parse_options():
    # 命令行解析函数
    parser = argparse.ArgumentParser()

    parser.add_argument('-f', '--file', default='TestData.csv', help='dump文件名')

    args = parser.parse_args()
    return args


if __name__ == '__main__':
    args = parse_options()
    csv_data_analysis = DataAnalysis(args.file)
    print(csv_data_analysis)
