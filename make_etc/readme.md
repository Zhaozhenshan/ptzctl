#### 使用方式1

./test.py -f=test.csv  
此种方式执行文件不做任何事情

#### 使用方式2

./test.py -f=test.csv -m=true
执行后生成etc目录，每个目录下有etc和dui两个文件夹

#### 使用方式3

./test.py -f=test.csv -s=1
./test.py -f=test.csv -s=1-169
执行后在子etc目录里生成配置文件，目前为解决文件名冲突，暂时写成.json文件

