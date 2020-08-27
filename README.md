# USV_obstacle_avoidance
无人船避障模块，使用激光雷达或毫米波作为避障监测传感器挂载在一块STM32F103协处理器上使用。

****
##  一. 基本说明
  ### 文件目录
  * OA-without-OS目录下存放无操作系统版本的避障工程
  * OA-with-OS目录下存放基于freertos操作系统的避障工程
  * *两工程各自独立，分别烧录均可运行*
  
  
  ### 避障方案
  
  **注意这里的传感器只是本人在开发过程中使用到的传感器型号，仅供参考；使用其他传感器只需改一下串口协议即可！**
  
  * 方案一：激光雷达1个+舵机1个：舵机带动激光雷达转动，扫描障碍物（已实现√）{成本：300元}
  * 方案二：仅使用一个单发单收天线毫米波雷达（无法测角，仅能测量船头方向的较大较远的障碍物）（已实现√）{成本：500~1000元}
      * 不建议此方案
  * 方案三：仅使用一个单发多收天线毫米波雷达（可以测角，待测试）{成本：1200以上}
  * 方案四：正前方安装一个毫米波雷达+两侧各一个激光雷达（可提高避障识别范围，但成本高）{1200+200 * 2}
  * 方案五：船头5个角度各安装一个激光雷达，共5个{成本：1000元}
  * 方案六：正前方安装一个毫米波雷达与一个激光雷达，两侧各一个激光雷达{1200+2 * 200}
      * 24Ghz毫米波适合测量远距离障碍物，对近距离障碍较无力。
      * 77Ghz\79Ghz对近距离障碍物较有效。
  * 方案七：步进电机+毫米波或激光雷达，步进电机作为转动结构；
      * 步进电机在工业机械臂、3D打印机等转动结构上应用的很多，比舵机稳定、精确。这里只是一个构思方案。
  ### 已实现方案的硬件配置
  * 方案一：
    * 激光雷达1个：北醒® TFmini Plus，单线防水串口雷达。价格230左右一个。
    * 舵机1个（配合激光雷达进行扫描）：普通舵机即可，控制协议为pwm：周期20ms，占空比0.5ms~2.5ms
  * 方案二[不建议]：
      * 单发单收天线毫米波雷达1个
      * 实验传感器：英泰雷® S2毫米波雷达，24Ghz无测角，范围1~100m。价格800。买了一个做测试，达不到宣称的效果。
  * 方案三：
      * 单发多收天线毫米波雷达1个
      * 实验传感器：纳雷科技SP70C毫米波雷达,24Ghz可测角大约100度，范围0.1~40m。价格1200.
  * 控制板：STM32F103RCTx
  ### 软件开发环境
  * STM32使用HAL库编写（兼容性强）
  * 硬件管脚配置使用STM32Cube MX（如果会使用HAL库的话用不用这个无所谓）
  * 注：避障由于有比导航更高的优先级，可以考虑上操作系统，例如freertos。
****
## 二. 避障算法说明
  ### 1.基本说明
  1. 目前此避障模块仅初步完成，还未与主处理器或导航算法进行融合。可以将其视为一个避障小车。
  2. 由于船的作业条件比较复杂，需要考虑水汽等影响。经过测试，虽然超声波模块理论可行，但实际上超声波的数据非常不稳定，舍弃。
  3. 激光雷达效果和超声波模块基本类似，但是激光雷达数据稳定，在天气晴朗的情况下效果很好；缺点是容易受水汽影响。
  4. 毫米波雷达比较理想，单天线毫米波雷达可以探测多个障碍物的距离、速度数据；多天线的毫米波可以在此之上输出障碍物的角度数据。但是一般使用24Ghz毫米波雷达的电磁波波长为约13cm，也就是说基本直径在13cm以内的障碍物它是监测不到的，从而水汽、下雨天对其影响不大；其次毫米波雷达一般对大型、比较远距离的障碍物反应灵敏，比如道路上的汽车等，对近距离、小型障碍物反应不够灵敏。要想避免这些问题，需要使用高频，波长更短的毫米波雷达。一般使用77Ghz的。但其成本非常高，一台77Ghz毫米波雷达售价为3000/4000左右；同时如果想要输出障碍物的角度信息则需要多天线毫米波雷达，其售价同样高昂（3000/5000RMB）
  5. 目前基于成本与性能考虑，采取的是激光+舵机方案，同时也在开发毫米波方案，采取条件编译。
  ### 2.避障思路（激光雷达+舵机方案）
  1. 激光雷达架设在舵机上，让舵机围绕船头正前方左右对称地旋转扫描，共20个旋转位置（对应舵机控制的高电平时间0.5ms~2.5ms）
  2. 每个旋转位置等效的看作一个激光雷达输出结果，也就是说将舵机+激光雷达等效的看作是20个激光雷达。
  3. 对20个输出结果进行加权平均，最后合成为5个数据，分别对应左、左前、前、右前、右，5个方向。
  4. 总之就是用旋转的一个激光雷达模拟成5个不同角度的激光雷达。相当于搭了个框架；
  5. 以后就算激光+舵机不好用，也可以考虑用5个或者3个固定位置的激光可以达到基本一样的避障效果。
  6. 计划：测试好了毫米波之后试试能不能用毫米波一下子模拟[左前、前、右前]三个方向的激光雷达。
  ### 3.避障函数输出
  1. 经过封装，避障检测函数可以输出一个5元数组，分别对应上面提到的5个方向的距离数据
  2. 避障反应函数可根据上面的数组做出相应的反应，例如：当中间三个方向没有检测到障碍物时可以通行。
  3. 避障反应函数可以直接调用电机控制函数，在检测到障碍物时即时作出反应。
  ### 4.STM32资源对应表
  |板载资源、引脚|对应传感器、外围设备接口|
  |---|---
  |TIM3 CH1|舵机pwm信号线
  |TIM3 CH2|左电机ENA
  |TIM3 CH3|右电机ENA
  |USART1|与电脑连接，调试显示串口
  |USART2|激光雷达串口
  |USART3|毫米波雷达串口
  |PC6/7/8/9|电机驱动板IO1/2/3/4
  
  ****
## 三. 其他说明
* 计划将避障模块做成一个挂载在协处理器(STM32F103RCTx)上，再与主处理器STM32F103ZETx或其他32控制板相连。  
* [主处理器STM32程序(标准库)Github地址](https://github.com/matreshka15/unmanned-ship-stm32-part)
* [上位机端Python版Github地址](https://github.com/matreshka15/raspberry-pi-USV-program)
* [上位机端ROS版Github地址](https://github.com/matreshka15/ROS-based-unmanned-vehicle-project)
* [姿态解算算法的解释与实机演示视频(早期,不包含避障)](https://zhuanlan.zhihu.com/p/82973264)

### 部分开发下位机过程中的开发日志以及手册均已存放在下面地址
* [开发无人船过程中参考的传感器手册以及算法资料](https://github.com/matreshka15/unmanned-ship-datasheets)
* 开发日志记录了项目从一开始的立项到后面一步步测试成功的大大小小细节。前后由于放弃了旧的姿态算法、选取了新的姿态算法，因此前期关于姿态的说明仅供参考用。
* 避障算法未包含在其中。
