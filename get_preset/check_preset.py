#!/usr/bin/env python3
import pandas as pd
import requests
from requests.auth import HTTPDigestAuth


class CameraPresetInfo:
    ''' 发送Get请求, 检查预置位信息是否正确

    param1: ip
    param2: DigestAuth_username
    param3: DigestAuth_password
    '''

    def __init__(self, ip, username, password) -> None:
        self.camera_ip = ip
        self.username = username
        self.password = password

        self.get_preset_url = 'http://' + ip + '/LAPI/V1.0/Channels/0/PTZ/Presets'
        self.data = {}
        self.get_preset()

    def get_preset(self) -> bool:
        ''' 发送Get请求, 获取预置位信息, 存到self.data

        存储结果分成以下三种: data = "null" ; data = {} ; data = {正常数据}
        '''
        try:
            result = requests.get(self.get_preset_url, auth=HTTPDigestAuth(
                self.username, self.password), timeout=3)

            if result.status_code == 200:
                self.data = result.json()['Response']['Data']
                return True
            else:
                return False

        except Exception:
            return False

    def check_preset(self) -> bool:
        ''' 检查获取到的预置位信息

        True,当前仅当预置位信息包含 7,100,101,102 四个预置位
        '''
        if not bool(self.data) or self.data == 'null':
            return False

        preset_id_set = set()
        for preset in self.data['PresetInfos']:
            preset_id_set.add(preset["ID"])

        check_set = {7, 100, 101, 102}
        if (preset_id_set & check_set) == check_set:
            return True
        return False

    def get_preset_num(self) -> int:
        if self.data:
            if self.data != 'null':
                return int(self.data['Nums'])
            else:
                return -1
        else:
            return -1


def print_preset_info(cameras_list):
    ''' 循环打印预置位信息, 汇总预置位出错的设备信息

    param: 维度为 n * 2 的列表. [[桩号], [ip]]
    '''

    error_list = []

    for i in range(len(cameras_list[1])):
        presetInfo = CameraPresetInfo(cameras_list[1][i], "admin", "Ab123456")

        result = str(cameras_list[0][i]).ljust(10) \
            + str(presetInfo.camera_ip).ljust(15) \
            + "Preset_Nums:" + str(presetInfo.get_preset_num()).ljust(4) \
            + str('\033[1;32mCorrect\033[0m' if presetInfo.check_preset()
                  else '\033[1;31mError\033[0m')

        print(result)
        if presetInfo.check_preset() == False:
            error_list.append(result)

    print('*' * 60)
    print("Error numbers: ", len(error_list))
    for x in error_list:
        print(x)


def get_camera_list(file_name):
    ''' 读取CSV文件数据

    param: csv文件名称
    return: 维度为 n * 2 的列表. [[桩号], [ip]]
    '''

    data = pd.read_csv(file_name)
    cameras_ip_list = list(data["ip"])
    cameras_pile_list = list(data["device_name"])

    cameras_list = [[], []]
    for i in range(len(cameras_pile_list)):
        cameras_pile_list[i] = str(cameras_pile_list[i]).split("-")[0]
        cameras_list[0].append(cameras_pile_list[i])
        cameras_list[1].append(cameras_ip_list[i])

    return cameras_list  # [ [桩号], [ip] ]


if __name__ == '__main__':
    cameras_list = get_camera_list('ball_cameras.csv')
    print_preset_info(cameras_list)
