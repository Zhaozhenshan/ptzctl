#!/usr/bin/env python3

import argparse
import os
import json
import re


def readcsv(file_name) -> list:
    '''
        根据file_name读取csv文件并进行预处理

        :param: file_name, csv文件名称
        :return: data, 去除结尾的\n, 每一行数据用spilt函数处理
    '''
    try:
        fd = open(file_name, 'r')
        data = fd.readlines()
        for i in range(len(data)):
            data[i] = data[i].replace('\n', '')
            data[i] = data[i].split(',')
        return data
    except Exception:
        print('读取文件失败')
        return []


def data_filling(data) -> list:
    '''
        对特定的列进行补全，去除前面特定的行数

        operator1: col[0],补全点位编号  col[2],补全桩号  col[3],补全安装位置
        operator2: 表头信息, 过滤掉
    '''
    # 找到首行数据的行号
    start_row = 0
    for i in range(len(data)):
        if data[i][0] == '1':
            start_row = i
            break
    data = data[start_row:]

    # 对 0 2 3 列数据进行补全
    col = (0, 2, 3)
    for i in range(len(data)):
        for j in col:
            if data[i][j] == '':  # 向上对齐
                data[i][j] = data[i-1][j]
    return data


def data2group(data) -> list:
    # 根据点位编号对数据进行分组
    if len(data) == 0:
        return []

    group = [[]]
    group[-1].append(data[0])

    for i in range(1, len(data)):
        if data[i][0] != group[-1][0][0]:
            group.append([])
        group[-1].append(data[i])

    return group


def read_data(file_name):
    # 组合读取数据的多个函数
    data = readcsv(file_name)
    data = data_filling(data)
    data = data2group(data)
    return data


class MakeDir:
    '''
        创建etc目录
        param: data, 分组后的数据列表
    '''

    def __init__(self, data) -> None:
        self.data = data
        self.dir_list = self.get_dir()

    def get_dir(self) -> list:
        # 获取目录列表, 例如 : 001-K2_800-172.21.4.6
        data = self.data
        dir = []
        for i in range(len(data)):
            for line in data[i]:
                if line[4] == 'mec':
                    dir.append(
                        str(line[0]).rjust(3, '0') + '-' + str(line[2]).replace('+', '_') + '-' + str(line[6]))
        return dir

    def make_dir(self) -> None:
        # 根据目录列表创建目录
        cwd = os.path.join(os.getcwd(), 'etc')
        os.makedirs(cwd)
        for per_dir in self.dir_list:
            station_path = os.path.join(cwd, per_dir)
            os.makedirs(station_path)
            os.makedirs(os.path.join(station_path, 'dui'))
            os.makedirs(os.path.join(station_path, 'etc'))


class Device:
    def __init__(self, data) -> None:
        '''
            设备的详细信息
            目前设备信息除station_id为int类型之外, 其他字段默认为字符串类型, 设置属性时需要类型转换
        '''
        self.station_id = int(data[0])
        self.region, self.pipe_number, self.install_position = data[1], data[2], data[3]
        self.type, self.direction, self.device_ip = data[4], data[5], data[6]
        self.utm_x, self.utm_y, self.utm_zone = data[7], data[8], data[9]
        self.longitude, self.latitude, self.height = data[10], data[11], data[12]
        self.serial, self.model = data[13], data[14]
        self.net_mask, self.gateway = data[15], data[16]


class Station():
    '''
        存储每个点位的所有设备信息
        param: station, 同一点位的设备列表
    '''

    def __init__(self, station) -> None:
        self.devices = {}  # 设备信息
        self.station_id = -1  # 点位编号
        self._precursor = None  # 前驱
        self._successor = None  # 后继
        self.data_handle(station)
        self.etc_dir = str(self.station_id).rjust(3, '0') \
            + '-' + str(self.get_by_key('mec').pipe_number).replace('+', '_') \
            + '-' + str(self.get_by_key('mec').device_ip)  # etc目录名

    def data_handle(self, station):
        # 遍历列表, 设置 devices 和 station_id
        for data in station:
            device = Device(data)
            key = str(device.type)
            self.devices[key] = device

            if self.station_id == -1:
                self.station_id = device.station_id
        del self.devices['交换机']

    def get_by_key(self, key) -> Device:
        # 根据key获取具体的device
        if key in self.devices.keys():
            return self.devices[key]
        else:
            raise KeyError('设备关键字错误')

    def print_device_keys(self):
        # 打印该点位所有设备的key
        print(self.devices.keys())

    '''
        利用property装饰器设置_precursor(前驱)和_successor(后继)的setter和getter方法
        作用是: 调用setter方法时, 能检查value是否为Station类型
    '''
    @property
    def precursor(self):
        return self._precursor

    @precursor.setter
    def precursor(self, value):
        if isinstance(value, Station):
            self._precursor = value
        else:
            raise TypeError('Error: 不是Station类型')

    @property
    def successor(self):
        return self._successor

    @successor.setter
    def successor(self, value):
        if isinstance(value, Station):
            self._successor = value
        else:
            raise TypeError('Error: 不是Station类型')


class StationManager:
    ''' 建立每个点位的所有信息, 初始化每个点位的前驱和后继节点
        param: station_list , shape: (m ✖️ n) , m为点位号, n为每个点位内的设备数量
    '''

    def __init__(self, station_list) -> None:
        self.stations = []
        self.data_handle(station_list)

    def data_handle(self, station_list) -> None:
        # 建立每个点位的Station对象, 存储到self.stations列表
        for per_station in station_list:
            self.stations.append(Station(per_station))

        # 根据点位数分情况处理点位的前驱和后继
        if len(self.stations) == 0:
            return
        elif len(self.stations) == 1:
            self.stations[0].precursor = self.stations[0]
            self.stations[0].successor = self.stations[0]
        else:
            self.stations[0].successor = self.stations[1]
            self.stations[-1].precursor = self.stations[-2]
            for i in range(1, len(self.stations) - 1):
                self.stations[i].precursor = self.stations[i-1]
                self.stations[i].successor = self.stations[i+1]

    def get_station_by_id(self, id) -> Station:
        # 根据id获取对应点位的Station对象
        if id >= 1 and id <= len(self.stations):
            return self.stations[id-1]
        else:
            error_info = '索引序号 ' + \
                str(id) + ' 超出范围: [1,' + str(len(self.stations)) + ']'
            raise IndexError(error_info)


def gen_conf_file(station: Station):
    # 配置文件所在的路径
    etc_path = os.path.join(os.getcwd(), 'etc', station.etc_dir, 'etc')

    # step1: 配置sensor.cfg
    with open(os.path.join(etc_path, 'sensor.cfg')) as sensor_fd:
        sensor_conf = json.load(sensor_fd)

    sensor_conf['cameraInfos'] = []
    sensor_conf['radarInfos'] = []
    for key in station.devices.keys():
        prop = {}
        device = station.get_by_key(key)

        if str(key)[0:-1] == 'camera':
            prop['sn'] = device.serial
            prop['deviceId'] = device.type
            prop['deviceIp'] = device.device_ip
            prop['devicePort'] = 80
            prop['deviceUser'] = 'admin'
            prop['devicePass'] = 'Ab123456'
            prop['deviceType'] = 1
            sensor_conf['cameraInfos'].append(prop)
        elif str(key)[0:-1] == 'radar':
            prop['enable'] = True
            prop['sn'] = device.serial
            prop['deviceType'] = 'stj'
            prop['deviceId'] = device.type
            prop['deviceIp'] = device.device_ip
            prop['devicePort'] = 8008
            sensor_conf['radarInfos'].append(prop)

    with open(os.path.join(etc_path, 'sensor.json'), 'w') as sensor_fd:
        json.dump(sensor_conf, sensor_fd, indent=4)

    # step2: 配置cross.cfg
    with open(os.path.join(etc_path, 'cross.cfg')) as cross_fd:
        cross_conf = json.load(cross_fd)

        # question: 第一个点位前驱信息怎么配置?
    cross_conf[0]['deviceId'] = str(station.station_id)
    if station.precursor:
        cross_conf[0]['upstreamDeviceId'] = str(station.precursor.station_id)
        cross_conf[0]['upstreamDeviceAddr'] = str(
            'tcp://ip:5555').replace('ip', station.precursor.get_by_key('mec').device_ip)

    with open(os.path.join(etc_path, 'cross.json'), 'w') as cross_fd:
        json.dump(cross_conf, cross_fd, indent=4)

    # step3: 配置algoentry.cfg
    with open(os.path.join(etc_path, 'algoentry.cfg')) as algoentry_fd:
        algoentry_conf = json.load(algoentry_fd)

    algoentry_conf['cameraInfos'] = []
    algoentry_conf['radarInfos'] = []
    for key in station.devices.keys():
        prop = {}
        device = station.get_by_key(key)

        if str(key)[0:-1] == 'camera':
            prop['cameraType'] = 0
            prop['cezhuangDianJing'] = 0
            prop['deviceId'] = device.type
            prop['enuX'] = float(device.utm_x) if device.utm_x != '' else 0.0
            prop['enuY'] = float(device.utm_y) if device.utm_y != '' else 0.0
            prop['intrinsicsPath'] = './config/haidian_101/params/camera1_intrinsics.yaml'
            prop['isStream'] = False
            prop['upstreamId'] = device.type
            prop['url'] = str(
                'rtsp://admin:Ab123456@ip/media1').replace('ip', device.device_ip)
            algoentry_conf['cameraInfos'].append(prop)
        elif str(key)[0:-1] == 'radar':
            prop['deviceId'] = device.type
            prop['enuX'] = float(device.utm_x) if device.utm_x != '' else 0.0
            prop['enuY'] = float(device.utm_y) if device.utm_y != '' else 0.0
            algoentry_conf['radarInfos'].append(prop)

    with open(os.path.join(etc_path, 'algoentry.json'), 'w') as algoentry_fd:
        json.dump(algoentry_conf, algoentry_fd, indent=4)


def parse_options():
    # 命令行解析函数
    parser = argparse.ArgumentParser()
    parser.add_argument('-f', '--file', default='TestData.csv',
                        required=True, help='dump文件名')
    parser.add_argument('-s', '--station', default='0', help='点位范围')
    parser.add_argument('-m', '--makedir', default='false',
                        choices=['true', 'false'], help='是否生成目录')
    args = parser.parse_args()
    return args


def action():
    # 解析命令行参数
    args = parse_options()
    # 读取csv文件并格式化数据
    data = read_data(args.file)
    # 构造stationManager
    stationManager = StationManager(data)
    # 构造mkdir, 用于生成etc目录
    mkdir = MakeDir(data)

    # 是否生成目录
    if args.makedir == 'true':
        mkdir.make_dir()  # 生成目录

    # 解析并处理station参数
    s = str(args.station)
    if s == '0':
        pass
    elif s.find('-') == -1:
        id = int(s)
        gen_conf_file(stationManager.get_station_by_id(id))
    else:
        left, right = s.split('-')
        left, right = int(left), int(right)
        for id in range(left, right + 1):
            gen_conf_file(stationManager.get_station_by_id(id))


if __name__ == '__main__':
    action()
