#!/usr/bin/env python3

import os
import subprocess
import argparse
import random
import string
import builtins

kk_sid_set = {
    1,3,5,7,8,10,12,14,16,17,20,22,24,27,29,30,31,34,36,37,38,41,43,45,47,50,51,53,55,57,59,
    61,63,65,67,68,71,73,75,77,79,81,82,84,86,88,90,92,94,96,98,100,102,104,106,108,110,111,113,115,117,119,
    122,124,126,128,130,132,134,136,138,141,142,143,146,148,149,151,153,156,160,162,165,167
}

class ExecutorResultSummary():
    _instance = None
    _flag = False
    def __new__(cls, *args, **kwargs):
        if cls._instance is None:
            cls._instance=super().__new__(cls)
        return cls._instance

    def __init__(self):
        if not ExecutorResultSummary._flag:
            ExecutorResultSummary._flag = True
            self.success = {}
            self.failure = {}

    def save(self, device, code, rstr):
        rstr = rstr.replace('\r\n', '')
        rstr = rstr.replace('\n', '')
        if code == 0:
            if rstr not in self.success:
                self.success[rstr] = []
            self.success[rstr].append(device.id)
        else:
            if rstr not in self.failure:
                self.failure[rstr] = []
            self.failure[rstr].append(device.id)

    def print(self):
        print('\n[SUMMARY]')
        self.print_failure()
        self.print_success()

    def print_failure(self):
        cnt = 0
        summary_info = ''
        for k, v in self.failure.items():
            cnt = cnt + len(set(v)) 
            summary_info = summary_info + '    ERROR:' + k +'\n      RefStations:'
            ids = list(set(v))
            ids.sort()
            for id in ids:
                summary_info = summary_info + str(id) + ' '
            summary_info = summary_info + '\n'
        print('  FAILURE COUNT:', cnt)
        print(summary_info)

    def print_success(self):
        cnt = 0
        summary_info = ''
        for k, v in self.success.items():
            cnt = cnt + len(set(v)) 
            ids = list(set(v))
            ids.sort()
            for id in ids:
                summary_info = summary_info + str(id) + ' '
        print('  SUCCESS COUNT:', cnt)
        print('    RefStations:' + summary_info)

class CmdExecutor:
    def __init__(self, cmd):
        self.cmd = cmd

    def run(self):
        if args.test:
             print('  EXEC:\"' + self.cmd + '\";')
             return 0, 'Just a test'

        if args.verbose:
            print('  EXEC:\"' + self.cmd + '\";', end = '', flush = True)
        result = subprocess.run(self.cmd, shell=True, stdout = subprocess.PIPE, stderr = subprocess.STDOUT)
        if args.verbose:
            print('RESULT:' + str('\033[1;32mSUCC\033[0m' if 0 == result.returncode else '\033[1;31mFAIL\033[0m'))  
        rstr = str(result.stdout, 'UTF-8')
        if 0 != len(rstr):
            print('    ' + rstr.replace('\n', '\n    '))
        return result.returncode, rstr

class CopyExecutor:
    def __init__(self, device, host, remote, user, passwd, options = ''):
        self.device = device
        self.host = host
        self.remote = remote
        self.user = user
        self.passwd = passwd
        self.options = options

    def copy_from_device(self):
        src = self.user + '@' + self.device.ip + ':' + self.remote
        dst = self.host
        self.__do_copy__(src, dst)

    def copy_to_device(self):
        src = self.host
        dst = self.user + '@' + self.device.ip + ':' + self.remote
        self.__do_copy__(src, dst)

    def __do_copy__(self, src, dst):
        cmd = 'sshpass -p ' + self.passwd + ' rsync -az --timeout 3 ' + self.options + ' ' + src + ' ' + dst
        execor = CmdExecutor(cmd)
        code, str = execor.run()
        summary = ExecutorResultSummary() 
        summary.save(self.device, code, str)

class RemoteCommandExecutor:
    def __init__(self, device, cmd, user, passwd):
        self.device = device
        self.cmd = device.fix_cmd(cmd)
        self.user = user
        self.passwd = passwd

    def run(self):
        remote = self.user + '@' + self.device.ip
        cmd = 'sshpass -p ' + self.passwd + ' ssh -o StrictHostKeyChecking=no -o ConnectTimeout=3 ' + remote + ' \"' + self.cmd + '\"'
        execor = CmdExecutor(cmd)
        code, str = execor.run()
        summary = ExecutorResultSummary() 
        summary.save(self.device, code, str)

class LocalCommandExecutor:
    def __init__(self, device, cmd):
        self.device = device
        self.cmd = device.fix_cmd(cmd)

    def run(self):
        execor = CmdExecutor(self.cmd)
        code, str = execor.run()
        summary = ExecutorResultSummary() 
        summary.save(self.device, code, str)

class MecDevice:
    def __init__(self, dir, user, passwd) :
        self.directory = dir
        self.id, self.rod, self.ip = dir.split('/')[-1].split('-')
        self.id = int(self.id)
        self.user = user
        self.passwd = passwd
        self.calis = set()

    def info(self):
        return (str(self.id) + '#' + self.rod + '#' + self.ip)
    
    def fix_cmd(self, cmd):
        return cmd.replace('${ID}', str(self.id).zfill(3)).replace('${IP}', self.ip).replace('${ROD}', self.rod)

    def copy_from_device_etc(self, etcs):
        os.makedirs(self.directory + '/dui', exist_ok = True)
        copyer = CopyExecutor(self, self.directory, '/home/mec/dui', self.user, self.passwd, '-L')
        copyer.copy_from_device()

        os.makedirs(self.directory + '/etc', exist_ok = True)
        for etc in etcs:
            copyer = CopyExecutor(self, self.directory + '/etc', '/home/mec/etc/' + etc, self.user, self.passwd)
            copyer.copy_from_device() 

    def copy_from_device(self, host, remote):
        copyer = CopyExecutor(self, host, remote, self.user, self.passwd)
        copyer.copy_from_device()

    def copy_to_device_etc(self):
        self.copy_to_device(self.directory + '/*', '/home/mec/')

    def copy_to_device(self, host, remote):
        copyer = CopyExecutor(self, host, remote, self.user, self.passwd)
        copyer.copy_to_device()

    def run_remote_cmd(self, cmd):
        execor = RemoteCommandExecutor(self, cmd, self.user, self.passwd)
        execor.run()

    def run_local_cmd(self, cmd):
        execor = LocalCommandExecutor(self, cmd)
        execor.run()

    def add_cali_file(self, cali):
        self.calis.add(cali)

    def deploy_califile(self, cmd):
        summary = ExecutorResultSummary() 

        if self.id not in kk_sid_set:
            print('Could not deploy calibrations for ' + str(self.id))
            summary.save(self, 1, 'Not a kk station')
            return

        tmpdir = './tmp/'
        CmdExecutor('mkdir ' + tmpdir).run()
        for ca in self.calis:
            CmdExecutor('unzip -d ' + tmpdir + ' ' + ca + ' > /dev/null').run()
            sub1 = ca.replace(".", "-").split('-')[-2]
            sub2 = sub1.split('_')[0]
            CmdExecutor('cp -r ' + tmpdir + '/' + sub1 + '/' + sub2  + '/* ' +  self.directory + '/etc/config/calibration/' ).run()
        CmdExecutor('rm -fr ' + tmpdir).run()
        summary.save(self, 0, 'success')

def make_device_list(etc_dir):
    devices = {}
    for sub_dir in os.listdir(etc_dir):
        dev = MecDevice(os.path.join(etc_dir, sub_dir), args.user, args.passwd)
        devices[dev.id] = dev
    return devices

def caution_on_this_action(cs = 8):
    rdata = ''.join(random.sample(string.ascii_letters + string.digits, cs))
    idata = builtins.input('Please input the same characters [' + rdata +']:')
    if rdata != idata:
        print('Check failure! exit...')
        exit()

def get_actions():
    actions = {
        'from':  [ lambda dev : dev.copy_from_device(args.host, args.remote), False ],
        'to':    [ lambda dev : dev.copy_to_device(args.host, args.remote), False ],
        'fetc':  [ lambda dev : dev.copy_from_device_etc(["*.cfg", "install", "config"] if dev.id in kk_sid_set else ["*.cfg", "install"]), False ],
        'tetc':  [ lambda dev : dev.copy_to_device_etc(), True ],
        'rexec': [ lambda dev : dev.run_remote_cmd(args.cmd), False ],
        'lexec': [ lambda dev : dev.run_local_cmd(args.cmd), False ],
        'cali' : [ lambda dev : dev.deploy_califile(args.cmd), False ],
        'fcmd' : [ lambda dev : print(dev.fix_cmd(args.cmd)), False ],
    }
    return actions

def parse_options():
    str2bool = lambda s : True if s.lower() in ('yes', 'true', 't', 'y', '1') else False

    parser = argparse.ArgumentParser()
    parser.add_argument('-s', '--scope', nargs='+', default = ['0'], help = '设置处理的设备ID')
    parser.add_argument('-d', '--dir', default='./project-jingha-etc/', help = '设置本地存放配置的目录')

    parser.add_argument('-u', '--user', default = 'baidu', help = '设置远程设备用户名')
    parser.add_argument('-p', '--passwd', default = 'b@iduv2x@jh', help = '设置远程设备密码')

    parser.add_argument('-a', '--action', choices = get_actions().keys(), default = 'rexec', help = '执行的动作')
    parser.add_argument('-c', '--cmd', default='ls -alh /home/mec', help ='执行的命令行,"-a rexec/lexec"时生效')
    parser.add_argument('-v', '--verbose', type = str2bool, default = True, help = '显示命令执行细节')

    parser.add_argument('--califiles', default = './califiles/', help = '标定文件存放目录')

    parser.add_argument('--host', default = './tmp/', help = '本地文件或文件夹, "-a from/to"时生效')
    parser.add_argument('--remote', default = '/home/baidu/tmp/', help = '远程文件或文件夹, "-a from/to"时生效')

    parser.add_argument('--summary', type = str2bool, default = True, help = '总结命令执行结果')
    parser.add_argument('--test', type = str2bool, default = False, help = '命令测试,仅显示要执行的命令行,不实际执行')

    args = parser.parse_args()

    # args.cmd = r + args.cmd

    ids = set()
    for elem in args.scope:
        id_pair = elem.split('-');
        if len(id_pair) == 1:
            ids.add(int(id_pair[0]))
        elif len(id_pair) == 2:
            minid = min(int(id_pair[0]), int(id_pair[1]))
            maxid = max(int(id_pair[0]), int(id_pair[1]))
            ids = ids | set(range(minid, maxid + 1));
        else:
            print("IGNORE SCOPE ELEMNET:" + elem)

    args.ids = list(ids)
    args.ids.sort()

    return args

def do_action(devices, actions, id):
    if id not in devices:
        print("Device with Id=" + str(id) + " do not exist.")
        return
    dev = devices[id]
    print('Operation \"' + args.action + '\" on ' + dev.info())
    actions[args.action][0](dev)

def setup_calibrations(calidir, devices):
    for cafile in os.listdir(calidir):
        id = int(cafile.split('-')[0])
        if id not in devices:
            print("Device with Id=" + str(id) + " do not exist.")
            continue
        devices[id].add_cali_file(os.path.join(calidir, cafile))

if __name__ == '__main__':
    args = parse_options()

    devices = make_device_list(args.dir)
    actions = get_actions()

    if args.action in set([ k if v[1] else None for k, v in actions.items()]):
        caution_on_this_action()

    if os.path.exists(args.califiles):
        setup_calibrations(args.califiles, devices)

    for id in args.ids:
        do_action(devices, actions, id)

    if args.summary:
        ExecutorResultSummary().print()
