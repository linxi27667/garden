#!/usr/bin/env python3
from docx import Document
from docx.shared import Pt, Inches, Cm, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.oxml.ns import qn
import os

OUTPUT_PATH = r'E:\MCU\esp32\xiaozhi\garden\云亭智联·大模型驱动的AIoT智慧庭院生态平台_论文_最终版.docx'

def set_cell_shading(cell, color):
    shading = cell._element.get_or_add_tcPr()
    shading_elem = shading.makeelement(qn('w:shd'), {
        qn('w:fill'): color,
        qn('w:val'): 'clear'
    })
    shading.append(shading_elem)

def add_heading(doc, text, level=1):
    heading = doc.add_heading(text, level=level)
    return heading

def add_paragraph(doc, text, bold=False, italic=False, color=None, alignment=None):
    p = doc.add_paragraph()
    run = p.add_run(text)
    if bold:
        run.bold = True
    if italic:
        run.italic = True
    if color:
        run.font.color.rgb = RGBColor(*color)
    if alignment:
        p.alignment = alignment
    return p

def add_image_placeholder(doc, description, size, caption):
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = p.add_run(f'【图片预留接口：{description}】')
    run.bold = True
    run.font.color.rgb = RGBColor(255, 0, 0)
    run.font.size = Pt(12)
    
    p2 = doc.add_paragraph()
    p2.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run2 = p2.add_run(f'请在此处插入图片，建议尺寸：{size}')
    run2.italic = True
    run2.font.color.rgb = RGBColor(128, 128, 128)
    
    p3 = doc.add_paragraph()
    p3.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run3 = p3.add_run(f'图片说明：{caption}')
    run3.italic = True
    run3.font.color.rgb = RGBColor(128, 128, 128)

def add_table(doc, headers, rows):
    table = doc.add_table(rows=len(rows)+1, cols=len(headers))
    table.style = 'Table Grid'
    
    for i, header in enumerate(headers):
        cell = table.rows[0].cells[i]
        cell.text = header
        for paragraph in cell.paragraphs:
            for run in paragraph.runs:
                run.bold = True
    
    for i, row in enumerate(rows):
        for j, cell_text in enumerate(row):
            table.rows[i+1].cells[j].text = cell_text
    
    return table

def create_thesis():
    doc = Document()
    
    # 封面
    for _ in range(3):
        doc.add_paragraph()
    
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = p.add_run('传智杯全国大学生IT技能大赛')
    run.bold = True
    run.font.size = Pt(26)
    
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = p.add_run('设 计 文 档')
    run.bold = True
    run.font.size = Pt(22)
    
    doc.add_paragraph()
    
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = p.add_run('云亭智联·大模型驱动的 AIoT 智慧庭院生态平台')
    run.bold = True
    run.font.size = Pt(18)
    run.font.color.rgb = RGBColor(44, 62, 80)
    
    doc.add_paragraph()
    
    # 项目信息表
    table = doc.add_table(rows=7, cols=2)
    table.style = 'Table Grid'
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    
    info_data = [
        ('项目内容', '说明'),
        ('作品名称', '云亭智联·大模型驱动的 AIoT 智慧庭院生态平台'),
        ('参赛赛道', '物联网应用开发'),
        ('团队名称', '云亭智联团队'),
        ('指导教师', '（请填写）'),
        ('团队成员', '（请填写）'),
        ('日期', '2025 年 5 月')
    ]
    
    for i, (key, value) in enumerate(info_data):
        table.rows[i].cells[0].text = key
        table.rows[i].cells[1].text = value
        if i == 0:
            for cell in table.rows[i].cells:
                for paragraph in cell.paragraphs:
                    for run in paragraph.runs:
                        run.bold = True
    
    doc.add_page_break()
    
    # 摘要
    add_heading(doc, '摘  要', level=1)
    
    add_paragraph(doc, '随着人工智能与物联网技术的深度融合，智慧家居正从室内单场景向庭院等开放式生活空间延伸。本文提出并实现了一种基于 ESP32-S3 微控制器的智慧庭院生态平台，创新性地采用"主机负责智能交互，从机负责实时执行"的主从协同架构。')
    
    add_paragraph(doc, '系统深度集成大语言模型驱动的语音交互、多模态传感器融合感知、基于 MCP（Model Context Protocol）的自然语言意图落地以及 MQTT 轻量级协议统一总线，实现了以下核心能力：')
    
    add_paragraph(doc, '（1）自然语言控制庭院设备，支持成语意境场景智能切换；（2）多传感器联动自动控制，包括土壤湿度自动浇水、温湿度自动调节、雨滴自动遮雨棚；（3）火灾安全本地闭环响应，采用 ADC 电压阈值轮询机制，不依赖云端与网络；（4）能耗追踪系统，基于额定功率×运行时间积分算法，实现各设备用电量精确统计；（5）AI 对话实时转发，支持多端对话同步。')
    
    add_paragraph(doc, '测试结果表明，该平台 MQTT 消息延迟低于 100ms，火灾检测响应时间 1.5 秒，系统具备低延迟、高可靠性与良好的可扩展性，为大模型时代智慧庭院的实际应用提供了一种具有前瞻性的技术方案。')
    
    p = doc.add_paragraph()
    run = p.add_run('关键词：')
    run.bold = True
    p.add_run('智慧庭院；AIoT；ESP32-S3；MQTT；大语言模型；MCP；语音交互；主从协同架构；多模态传感融合')
    
    doc.add_page_break()
    
    # 目录
    add_heading(doc, '目  录', level=1)
    
    toc_items = [
        '第一章  绪论',
        '  1.1  研究背景',
        '  1.2  国内外研究现状',
        '  1.3  本文工作与创新点',
        '  1.4  论文组织结构',
        '第二章  系统总体架构设计',
        '  2.1  系统架构概述',
        '  2.2  设计原则',
        '  2.3  技术栈选型',
        '  2.4  系统工作流程',
        '第三章  硬件设计',
        '  3.1  主机硬件设计',
        '  3.2  从机硬件设计',
        '  3.3  硬件安全设计',
        '  3.4  硬件调试与验证',
        '第四章  软件架构与分层设计',
        '  4.1  软件分层架构',
        '  4.2  BSP 层设计',
        '  4.3  APP 层设计',
        '  4.4  Application 层设计',
        '  4.5  MCP 工具层设计',
        '  4.6  任务调度与同步机制',
        '第五章  MQTT 通信协议设计',
        '  5.1  MQTT Broker 配置',
        '  5.2  Topic 设计',
        '  5.3  命令码体系',
        '  5.4  数据包结构',
        '  5.5  设备发现与心跳机制',
        '  5.6  通信安全设计',
        '第六章  智慧庭院业务功能',
        '  6.1  设备控制',
        '  6.2  成语意境场景系统',
        '  6.3  自动模式',
        '  6.4  传感器联动',
        '  6.5  火灾安全闭环',
        '  6.6  能耗追踪系统',
        '第七章  AI 语音交互与 MCP 工具集成',
        '  7.1  自研语音交互协议设计',
        '  7.2  MCP Server 架构',
        '  7.3  MCP 工具实现',
        '  7.4  对话实时转发',
        '  7.5  自然语言理解优化',
        '第八章  App 端接口设计',
        '  8.1  MQTT 接口规范',
        '  8.2  对话同步机制',
        '  8.3  App 端功能模块',
        '  8.4  数据可视化设计',
        '第九章  系统测试与总结',
        '  9.1  功能测试',
        '  9.2  性能测试',
        '  9.3  可靠性测试',
        '  9.4  总结与展望',
        '参考文献',
        '附录 A  完整代码清单',
        '附录 B  硬件原理图',
        '附录 C  测试数据表'
    ]
    
    for item in toc_items:
        add_paragraph(doc, item)
    
    doc.add_page_break()
    
    # 第一章 绪论
    add_heading(doc, '第一章  绪论', level=1)
    
    add_heading(doc, '1.1  研究背景', level=2)
    add_paragraph(doc, '当前，以大语言模型（LLM）为核心的人工智能技术正在重塑万物互联的交互范式，而物联网（IoT）技术的迅猛发展则为智能设备的普适化铺平了道路。两者的深度融合——即 AIoT（Artificial Intelligence of Things）——正在开启智慧生活的全新篇章。然而，现有智能家居解决方案大多聚焦于室内场景，对于庭院、花园等室外开放空间的智能化管理仍然是一片待开发的蓝海。')
    add_paragraph(doc, '庭院作为住宅与自然的过渡空间，承载着休闲娱乐、生态景观、安防监控等多重功能。传统庭院管理主要依赖人工操作，存在三大痛点：其一，灯光、浇灌、通风等设备需要人工逐一控制，操作繁琐且效率低下；其二，缺乏环境感知能力，无法根据天气、土壤湿度等条件自动调节设备状态；其三，安全防护能力不足，火灾等异常情况难以及时发现和处理。这些问题严重制约了庭院生活品质的提升，也与当下智慧化、自动化的时代趋势相悖。')
    
    add_heading(doc, '1.2  国内外研究现状', level=2)
    add_paragraph(doc, '在智能家居领域，国内外已涌现出诸多成熟解决方案。国外以 Amazon Alexa、Google Home、Apple HomeKit 为代表的智能语音助手平台，通过云端 AI 提供语音交互能力，已形成较为完善的生态体系。国内则有小米、华为等厂商推出的全屋智能方案，在设备互联与场景联动方面积累了丰富经验。然而，这些方案大多针对室内场景设计，对庭院等室外场景的覆盖有限，且多依赖封闭生态，可定制性与可扩展性不足。')
    add_paragraph(doc, '在庭院自动化领域，现有研究主要集中在智能浇灌系统和环境监测系统两个方面。智能浇灌系统通过土壤湿度传感器控制水泵，实现自动化浇灌；环境监测系统则通过温湿度、光照等传感器采集环境数据。但这些系统往往功能单一、彼此孤立，缺乏从感知到决策再到执行的全链路智能化设计，更未能融入大语言模型等新一代 AI 能力，难以满足用户对"说即达意"的智慧庭院期待。')
    
    add_heading(doc, '1.3  本文工作与创新点', level=2)
    add_paragraph(doc, '针对上述痛点与研究空白，本文提出并实现了一种基于 ESP32-S3 微控制器的智慧庭院生态平台，具有以下创新特色：')
    add_paragraph(doc, '（1）主从协同架构：主机专注于大模型语音交互与 AI 推理，从机专注于传感器感知与执行器控制，实现智能与实控的优雅分离。这种架构使得系统可以灵活扩展，新增设备只需在从机端接入，无需修改主机逻辑。')
    add_paragraph(doc, '（2）大模型驱动的自然语言控制：自研语音交互协议栈，集成 MCP 工具链，用户可通过自然语言无缝控制庭院设备，支持成语意境场景的智能切换，实现"说即达意"的智能体验。')
    add_paragraph(doc, '（3）多模态传感融合感知：集成土壤湿度、雨滴、火焰、温湿度等多类型传感器，实现跨模态数据融合与联动控制，构建了完整的"感知—决策—执行"闭环。')
    add_paragraph(doc, '（4）安全事件本地闭环：火灾检测采用本地 ADC 阈值轮询机制，不依赖云端与网络，确保极端情况下的安全性。系统实现了从检测、确认、响应到解除的完整安全闭环。')
    add_paragraph(doc, '（5）MQTT 统一总线：采用 MQTT 协议作为主机、从机、App 端的统一通信基础设施，实现多端协同互联，支持松耦合、高可靠的消息传输。')
    add_paragraph(doc, '（6）能耗追踪系统：创新性地实现基于软件的能耗追踪功能，无需额外硬件传感器即可实时监测各设备用电情况，支持 NVS 持久化存储和 MQTT 实时上报。')
    
    add_heading(doc, '1.4  论文组织结构', level=2)
    add_paragraph(doc, '本文共分为九章，各章内容安排如下：')
    add_paragraph(doc, '第一章为绪论，介绍研究背景、国内外研究现状、本文工作与创新点。')
    add_paragraph(doc, '第二章为系统总体架构设计，阐述系统架构概述、设计原则、技术栈选型和系统工作流程。')
    add_paragraph(doc, '第三章为硬件设计，详细介绍主机和从机的硬件设计、硬件安全设计和调试验证。')
    add_paragraph(doc, '第四章为软件架构与分层设计，描述 BSP 层、APP 层、Application 层和 MCP 工具层的设计。')
    add_paragraph(doc, '第五章为 MQTT 通信协议设计，包括 Broker 配置、Topic 设计、命令码体系和数据包结构。')
    add_paragraph(doc, '第六章为智慧庭院业务功能，涵盖设备控制、成语意境场景、自动模式、传感器联动、火灾安全闭环和能耗追踪。')
    add_paragraph(doc, '第七章为 AI 语音交互与 MCP 工具集成，介绍自研语音交互协议、MCP Server 架构和工具实现。')
    add_paragraph(doc, '第八章为 App 端接口设计，描述 MQTT 接口规范、对话同步机制和 App 端功能模块。')
    add_paragraph(doc, '第九章为系统测试与总结，展示功能测试、性能测试、可靠性测试结果，并进行总结与展望。')
    
    doc.add_page_break()
    
    # 第二章 系统总体架构设计
    add_heading(doc, '第二章  系统总体架构设计', level=1)
    
    add_heading(doc, '2.1  系统架构概述', level=2)
    add_paragraph(doc, '本系统创新性地采用"智能主机 + 执行从机"的协同架构，将 AI 推理与物理控制解耦部署。主机基于 ESP32-S3 搭载自研语音交互协议栈，负责语音唤醒、ASR 识别、LLM 推理、TTS 合成、MCP 工具调用以及 MQTT IoT 控制器功能；从机同样基于 ESP32-S3，专注于 GPIO/继电器/舵机/WS2812B 的实时刷新、ADC 与多种传感器的数据采集，以及本地安全联动逻辑。两者通过 Wi-Fi + MQTT 实现无缝通信，共同构成了一个完整的"感知—决策—执行"闭环。')
    
    add_image_placeholder(doc, '系统整体架构图', '800x600像素', '展示主机、从机、MQTT Broker、App端的协同关系，标注数据流向')
    
    add_heading(doc, '2.2  设计原则', level=2)
    add_paragraph(doc, '系统设计遵循以下核心原则，确保架构的清晰性、可靠性与可扩展性：')
    add_paragraph(doc, '（1）关注点分离：主机不直接连接庭院负载，庭院执行器全部连接从机。主机专注于智能交互与决策，从机专注于实时控制与数据采集，职责清晰、各司其职。这种分离使得系统可以灵活扩展，新增设备只需在从机端接入，无需修改主机逻辑。')
    add_paragraph(doc, '（2）场景与自动模式正交：意境场景管灯光氛围，自动模式管设备控制，两者互不干扰。唯一例外是安全警戒场景会强制关断危险设备，体现安全优先的设计理念。')
    add_paragraph(doc, '（3）安全事件本地闭环：火灾等安全事件首先在从机本地闭环处理，不依赖云端和主机，确保在网络异常时仍能保证安全。这种设计大大提高了系统的可靠性。')
    add_paragraph(doc, '（4）MQTT 统一总线：MQTT 是主机、从机、App 端的统一协议总线，所有设备通过 Topic 进行消息路由，实现松耦合、高可靠的多端通信。')
    
    add_heading(doc, '2.3  技术栈选型', level=2)
    add_paragraph(doc, '系统采用以下经过充分验证的技术栈，确保平台的稳定性与先进性：')
    
    add_table(doc, ['类别', '技术选型', '说明'], [
        ['硬件平台', 'ESP32-S3', '支持 Wi-Fi、蓝牙、I2S、RMT 等丰富外设'],
        ['开发框架', 'ESP-IDF v5.5.3', 'Espressif 官方物联网开发框架'],
        ['实时操作系统', 'FreeRTOS', '提供任务调度、信号量、事件组等机制'],
        ['通信协议', 'MQTT v3.1.1', '轻量级物联网消息传输协议'],
        ['语音交互', '自研语音协议栈', '支持唤醒词检测、ASR、TTS、LLM 全链路'],
        ['工具协议', 'MCP', 'AI 模型与外部工具的标准化接口']
    ])
    
    add_heading(doc, '2.4  系统工作流程', level=2)
    add_paragraph(doc, '系统的工作流程可以概括为以下几个阶段：')
    add_paragraph(doc, '（1）语音交互流程：用户说出唤醒词 → 麦克风采集音频 → ASR 语音识别 → LLM 推理生成回复和 MCP 工具调用 → TTS 语音合成 → 扬声器播放回复，同时执行设备控制。')
    add_paragraph(doc, '（2）自动控制流程：传感器周期性采集数据 → 从机本地判断是否触发联动 → 执行相应控制动作 → 通过 MQTT 上报状态变化 → 主机进行 TTS 播报。')
    add_paragraph(doc, '（3）安全响应流程：火灾传感器检测到异常 → ADC 阈值连续超限确认 → 从机本地执行安全响应（切断总闸、关闭水泵、打开照明）→ 通过 MQTT 发布紧急事件 → 主机进行 TTS 播报和蜂鸣器报警。')
    
    doc.add_page_break()
    
    # 第三章 硬件设计
    add_heading(doc, '第三章  硬件设计', level=1)
    
    add_heading(doc, '3.1  主机硬件设计', level=2)
    add_paragraph(doc, '主机硬件以 ESP32-S3 为核心，作为智慧庭院的"大脑"，主要连接以下外设，实现语音交互、状态显示与安全告警功能：')
    
    add_table(doc, ['模块', '引脚', '功能说明'], [
        ['I2S 麦克风', 'WS=GPIO1, SCK=GPIO2, DIN=GPIO42', 'INMP441 数字 MEMS 麦克风，语音输入'],
        ['I2S 扬声器', 'DOUT=GPIO39, BCLK=GPIO40, LRCK=GPIO41', 'MAX98357A 功放，语音输出'],
        ['OLED 显示屏', 'SDA=GPIO47, SCL=GPIO21', 'SSD1306 128×64 I2C 显示'],
        ['MPU6050', 'SDA=GPIO3, SCL=GPIO46', '六轴姿态传感器，软件 I2C'],
        ['蜂鸣器', 'GPIO14', '低电平有效，安全告警提示'],
        ['内置 LED', 'GPIO48', '系统状态指示']
    ])
    
    add_paragraph(doc, '主机采用 I2S 标准模式进行音频数据传输，配置为 16kHz 采样率、16-bit 位宽、单声道模式。麦克风采用 INMP441 数字 MEMS 麦克风，具备低噪声、高灵敏度的特点；扬声器通过 MAX98357A 功放驱动，实现高品质语音播报。通过软件 I2C 与 MPU6050 通信，实现姿态检测与跌倒报警功能。')
    
    add_image_placeholder(doc, '主机硬件连接示意图', '800x600像素', '展示ESP32-S3与麦克风、扬声器、OLED、MPU6050、蜂鸣器的连接关系')
    
    add_heading(doc, '3.2  从机硬件设计', level=2)
    add_paragraph(doc, '从机硬件同样基于 ESP32-S3，作为智慧庭院的"手脚"，连接以下执行器与传感器，实现环境感知与设备控制：')
    
    add_table(doc, ['设备', '逻辑名', '引脚', '类型', '功能说明'], [
        ['土壤湿度', 'soil_percent', 'GPIO1', 'ADC 输入', '干燥→低百分比，湿润→高百分比'],
        ['雨滴检测', 'rain_percent', 'GPIO4', 'ADC 输入', '雨水越多百分比越高'],
        ['西庭灯', 'courtyard_west_light', 'GPIO11', '数字输出', '小黄色 LED'],
        ['庭院灯带', 'strip_light', 'GPIO12', 'RMT 输出', 'WS2812B 30 颗 RGB 灯带'],
        ['长廊灯', 'corridor_long_light', 'GPIO13', '数字输出', '暖黄色 LED 灯带'],
        ['通风扇', 'fan', 'GPIO14', '继电器 CH0', '高电平导通'],
        ['水泵', 'water_pump', 'GPIO15', '继电器 CH1', '高电平导通'],
        ['雾化器', 'atomizer', 'GPIO16', '继电器 CH2', '高电平导通'],
        ['安防总闸', 'safety_switch', 'GPIO17', '继电器 CH3', '高电平导通，低电平切断'],
        ['智能遮雨棚', 'rain_shelter', 'GPIO18', 'LEDC PWM', '0°收回，90°展开'],
        ['DHT11 温湿度', 'temperature/humidity', 'GPIO38', '单总线', '温湿度采集'],
        ['火灾传感器', 'fire_status', 'GPIO2', 'ADC 输入', 'AO 引脚，电压越低火越近']
    ])
    
    add_paragraph(doc, '从机通过继电器模块控制通风扇、水泵、雾化器和安防总闸，采用 WS2812B 灯带作为氛围照明，舵机用于控制智能遮雨棚的展开和收回。硬件连接充分考虑了信号完整性与电气安全，确保系统在庭院环境下的长期稳定运行。')
    
    add_image_placeholder(doc, '从机硬件连接示意图', '800x600像素', '展示ESP32-S3与传感器、继电器、舵机、WS2812B灯带的连接关系')
    
    add_heading(doc, '3.3  硬件安全设计', level=2)
    add_paragraph(doc, '硬件安全设计是智慧庭院平台的重要基石，本系统从以下三个维度进行了深入考量：')
    add_paragraph(doc, '（1）安防总闸：采用高电平导通设计，上电默认 GPIO17=1，只有火灾确认或明确切断时 GPIO17=0。这种"安全导通"的设计理念确保了异常情况下的自动断电保护。')
    add_paragraph(doc, '（2）WS2812B 灯带：使用 5V 独立供电、共地、数据线串 330Ω~470Ω 电阻、入口并 1000µF 电容，长灯带使用 74AHCT125 电平转换，充分保障信号完整性。')
    add_paragraph(doc, '（3）火灾传感器：采用 ADC 电压阈值轮询替代传统中断模式，从根本上杜绝继电器 EMI 干扰导致的误报，大幅提升火灾检测的可靠性。')
    
    add_image_placeholder(doc, '实物照片', '1200x900像素', '实际搭建的智慧庭院硬件系统全景')
    
    add_heading(doc, '3.4  硬件调试与验证', level=2)
    add_paragraph(doc, '硬件调试过程中，我们遇到了以下几个关键问题并成功解决：')
    add_paragraph(doc, '（1）WS2812B 灯带信号完整性问题：初始设计中，灯带在长距离传输时出现颜色异常。通过在数据线串联 330Ω 电阻、入口并联 1000µF 电容，并采用 74AHCT125 电平转换芯片，成功解决了信号完整性问题。')
    add_paragraph(doc, '（2）火灾传感器误报问题：初期采用中断模式检测火焰，但继电器动作时的 EMI 干扰导致频繁误报。改为 ADC 电压阈值轮询模式后，通过连续 3 次超阈值确认机制，彻底解决了误报问题。')
    add_paragraph(doc, '（3）I2S 音频采样率问题：麦克风和扬声器的采样率不匹配导致音频失真。通过统一配置为 16kHz 采样率、16-bit 位宽，确保了音频质量。')
    
    doc.add_page_break()
    
    # 第四章 软件架构与分层设计
    add_heading(doc, '第四章  软件架构与分层设计', level=1)
    
    add_heading(doc, '4.1  软件分层架构', level=2)
    add_paragraph(doc, '系统软件采用经典的三层架构设计，从底层到顶层依次为 BSP 层、APP 层和 Application 层，各层职责明确、接口清晰，实现了高内聚、低耦合的软件架构。BSP 层负责硬件抽象，屏蔽底层硬件差异；APP 层负责业务逻辑，实现核心功能组装；Application 层负责任务调度与状态管理，驱动整个系统的运转。这种分层设计使得各层可以独立迭代、互不影响，显著提升了开发效率与系统可维护性。')
    
    add_image_placeholder(doc, '软件分层架构图', '800x600像素', '展示BSP层、APP层、Application层的层次关系和依赖方向')
    
    add_heading(doc, '4.2  BSP 层设计', level=2)
    add_paragraph(doc, 'BSP（Board Support Package）层是软件架构的基石，负责硬件抽象，提供统一的硬件操作接口，使上层代码与具体硬件实现解耦。主要包括：')
    add_paragraph(doc, '（1）bsp_wifi：WiFi 初始化与连接管理，封装 ESP-IDF 的 WiFi 驱动，提供阻塞式连接接口，确保网络连接的稳定性。')
    add_paragraph(doc, '（2）bsp_mpu6050：MPU6050 六轴传感器驱动，支持硬件 I2C 和软件 I2C 两种模式，灵活适配不同硬件布局。')
    add_paragraph(doc, '（3）bsp_buzzer：蜂鸣器控制，提供开关和蜂鸣接口，用于安全告警与状态提示。')
    add_paragraph(doc, '（4）bsp_iot_controller：IoT 控制器底层，管理远程设备表、处理响应解析和状态缓存，是多设备协同的核心支撑。')
    add_paragraph(doc, 'BSP 层采用函数指针的方式实现硬件抽象，例如 buzzer_t 结构体包含 Gpio_Init、Gpio_Write、Delay_ms 等函数指针，使得上层代码与具体硬件实现完全解耦，便于硬件平台的迁移与替换。')
    
    add_heading(doc, '4.3  APP 层设计', level=2)
    add_paragraph(doc, 'APP 层是业务逻辑的核心载体，将 BSP 层的硬件能力组装为可复用的业务模块，向上层提供高级业务接口。主要包括：')
    add_paragraph(doc, '（1）app_garden：智慧庭院核心业务模块，提供设备控制、场景切换、自动模式管理等接口，是平台业务逻辑的中枢。')
    add_paragraph(doc, '（2）iot_controller：MQTT 控制器，负责 MQTT 连接管理、消息收发、设备发现和心跳维护，是多端通信的桥梁。')
    add_paragraph(doc, '（3）app_fall_detect：跌倒检测模块，基于 MPU6050 加速度数据进行跌倒判断，提升庭院安全防护能力。')
    add_paragraph(doc, '（4）app_video_server：视频服务器，提供 HTTP MJPEG 流和 YOLO 推理框架，支持视觉 AI 能力的扩展。')
    add_paragraph(doc, '（5）app_mpu6050：MPU6050 应用层封装，提供初始化和数据读取接口，简化上层调用。')
    
    add_heading(doc, '4.4  Application 层设计', level=2)
    add_paragraph(doc, 'Application 层是系统的最高层，采用事件驱动的状态机架构，统筹调度整个系统的运行。主要特点包括：')
    add_paragraph(doc, '（1）事件驱动：使用 FreeRTOS 事件组实现事件通知，包括网络连接、唤醒词检测、VAD 变化、状态切换等事件，实现模块间的松耦合通信。')
    add_paragraph(doc, '（2）状态机：定义了设备状态枚举（Idle、Connecting、Listening、Speaking、Activating 等），通过 DeviceStateMachine 管理状态转换，确保系统行为的确定性。')
    add_paragraph(doc, '（3）任务调度：Schedule 方法允许其他任务将回调函数投递到主任务执行，确保线程安全，避免竞态条件引发的数据竞争。')
    add_paragraph(doc, '（4）协议抽象：Protocol 接口支持 MQTT 和 WebSocket 两种协议实现，通过 OTA 配置动态选择，实现通信方式的灵活切换。')
    
    add_heading(doc, '4.5  MCP 工具层设计', level=2)
    add_paragraph(doc, 'MCP（Model Context Protocol）工具层是大语言模型与物理世界的桥梁，负责将自然语言意图精准映射到设备控制动作。McpServer 采用单例模式管理所有注册工具，每个工具包含名称、描述、参数列表和处理函数。当云端发送 MCP 请求时，McpServer 解析参数并调用对应的处理函数，返回 JSON 格式的执行结果。这种设计使得新增设备只需注册新的 MCP 工具即可接入系统，具备极强的可扩展性。')
    
    add_heading(doc, '4.6  任务调度与同步机制', level=2)
    add_paragraph(doc, '系统采用 FreeRTOS 的任务调度机制，主要任务包括：')
    add_paragraph(doc, '（1）主任务（Main Task）：负责系统初始化、状态机管理和事件处理，是系统的核心调度中心。')
    add_paragraph(doc, '（2）MQTT 任务：负责 MQTT 连接维护、消息收发和设备发现，运行在独立的任务中。')
    add_paragraph(doc, '（3）传感器任务：负责 ADC 传感器数据采集和 DHT11 温湿度读取，周期性运行。')
    add_paragraph(doc, '（4）音频任务：负责麦克风数据采集和扬声器音频播放，需要高实时性。')
    add_paragraph(doc, '任务间通过 FreeRTOS 的队列、信号量和事件组进行同步，确保数据一致性和线程安全。')
    
    doc.add_page_break()
    
    # 第五章 MQTT通信协议设计
    add_heading(doc, '第五章  MQTT 通信协议设计', level=1)
    
    add_heading(doc, '5.1  MQTT Broker 配置', level=2)
    add_paragraph(doc, '系统使用自建 MQTT Broker 作为消息中转枢纽，支持 TCP 与 WebSocket 双通道接入，配置如下：')
    
    add_table(doc, ['配置项', '值'], [
        ['TCP 地址', 'mqtt://8.134.167.240:1883'],
        ['WebSocket 地址', 'ws://8.134.167.240:9001'],
        ['主机 Client ID', 'xiaozhi_master_esp32'],
        ['从机 Client ID', 'xiaozhi_slave_<MAC>'],
        ['Keepalive', '30 秒'],
        ['自动重连', '开启，重连间隔 5 秒']
    ])
    
    add_heading(doc, '5.2  Topic 设计', level=2)
    add_paragraph(doc, 'MQTT Topic 采用分层命名空间设计，以 xiaozhi 为根命名空间，清晰划分了命令下发、响应回传、状态同步、传感器上报、紧急事件与 AI 对话转发等功能域。')
    
    add_image_placeholder(doc, 'MQTT通信流程图', '800x600像素', '展示主机、从机、App端的消息发布订阅流程')
    
    add_table(doc, ['Topic', '方向', '格式', '功能说明'], [
        ['xiaozhi/iot/cmd/broadcast', '主机→从机', '二进制', '广播命令'],
        ['xiaozhi/iot/cmd/{device_id}', '主机→从机', '二进制', '单设备命令'],
        ['xiaozhi/iot/resp/{mac}', '从机→主机', '二进制', '命令响应'],
        ['xiaozhi/iot/heartbeat/{mac}', '从机→主机', '二进制', '心跳包'],
        ['xiaozhi/iot/announce/{mac}', '从机→主机', '二进制', '上线声明'],
        ['xiaozhi/iot/sensor/{mac}', '从机→主机', 'JSON/二进制', '传感器数据'],
        ['xiaozhi/iot/emergency/{mac}', '从机→主机', 'JSON', '紧急事件'],
        ['xiaozhi/garden/cmd', 'App→主机', 'JSON', '庭院高层命令'],
        ['xiaozhi/garden/status', '主机→App', 'JSON', '设备状态'],
        ['xiaozhi/garden/sensors', '主机→App', 'JSON', '传感器数据'],
        ['xiaozhi/garden/chat', '主机→App', 'JSON', 'AI 对话转发']
    ])
    
    add_heading(doc, '5.3  命令码体系', level=2)
    add_paragraph(doc, '系统定义了完整的二进制命令码体系，分为三个类别，兼顾功能完备性与传输紧凑性：')
    add_paragraph(doc, '基础命令（0x01-0x08）：IOT_CMD_SET_GPIO (0x01)、IOT_CMD_GET_GPIO (0x02)、IOT_CMD_HEARTBEAT (0x04)、IOT_CMD_RESPONSE (0x05)、IOT_CMD_DISCOVER (0x06)、IOT_CMD_ANNOUNCE_V2 (0x08)，覆盖设备发现、状态查询与心跳维护等基础功能。')
    add_paragraph(doc, '设备控制（0x10-0x13）：IOT_CMD_SET_SERVO (0x10)、IOT_CMD_SET_LIGHT (0x11)、IOT_CMD_SET_RELAY (0x12)、IOT_CMD_SENSOR_REPORT (0x13)，实现舵机、灯光、继电器的精细化控制与传感器数据上报。')
    add_paragraph(doc, '广播与场景（0x30-0x3B）：IOT_CMD_BROADCAST_ALL_OFF (0x30)、IOT_CMD_EMERGENCY (0x34)、IOT_CMD_SET_AUTO_MODE (0x35)、IOT_CMD_SET_STRIP_RGB (0x39)、IOT_CMD_SET_AMBIENCE_SCENE (0x3A)，支持全局广播、紧急联动、自动模式与意境场景切换。')
    
    add_heading(doc, '5.4  数据包结构', level=2)
    add_paragraph(doc, '系统定义了紧凑的二进制数据包结构，仅 8 字节，专为低带宽 IoT 场景优化：')
    add_paragraph(doc, 'typedef struct { uint8_t command; uint8_t device_id; uint8_t gpio_index; uint8_t value; uint8_t reserved[4]; } __attribute__((packed)) iot_command_packet_t;')
    add_paragraph(doc, '其中 reserved 字段用于扩展参数，例如设置灯带 RGB 时，reserved[0-2] 分别存储 R、G、B 值，reserved[3] 存储效果参数。这种紧凑而可扩展的设计，在保证传输效率的同时预留了充分的功能扩展空间。')
    
    add_heading(doc, '5.5  设备发现与心跳机制', level=2)
    add_paragraph(doc, '设备发现采用广播机制：从机上线后发送 IOT_CMD_ANNOUNCE_V2 命令到广播 Topic，携带设备名称和 GPIO 数量。主机收到后解析 MAC 地址，将设备动态添加到远程设备表，实现即插即用的设备接入体验。')
    add_paragraph(doc, '心跳机制用于维护设备在线状态：从机每 10 秒发送一次心跳包，主机收到后更新设备的 last_heartbeat_ms 时间戳。主机每 10 秒检查一次设备超时，超过 60 秒未收到心跳的设备标记为离线。这种双向心跳机制有效保障了系统对设备状态的实时感知能力。')
    
    add_heading(doc, '5.6  通信安全设计', level=2)
    add_paragraph(doc, '系统在通信安全方面采取了以下措施：')
    add_paragraph(doc, '（1）MQTT 连接认证：Broker 配置了用户名密码认证，防止未授权设备接入。')
    add_paragraph(doc, '（2）Topic 权限控制：通过 ACL 配置，限制不同客户端的 Topic 访问权限。')
    add_paragraph(doc, '（3）消息完整性：MQTT 协议本身支持 QoS 机制，确保消息的可靠传输。')
    add_paragraph(doc, '（4）设备身份验证：通过 MAC 地址绑定和设备发现机制，确保只有合法设备能够接入系统。')
    
    doc.add_page_break()
    
    # 第六章 智慧庭院业务功能
    add_heading(doc, '第六章  智慧庭院业务功能', level=1)
    
    add_heading(doc, '6.1  设备控制', level=2)
    add_paragraph(doc, '系统支持对庭院设备的精细化控制，实现了"语音即指令，设备即响应"的流畅体验：')
    add_paragraph(doc, '灯光控制：包括西庭灯（小黄色 LED）、庭院灯带（WS2812B RGB 灯带）和长廊灯（暖黄色 LED 灯带）。每路灯光可独立开关，灯带支持 RGB 颜色、亮度和效果（常亮、呼吸、彩虹、警示闪烁）的精确控制，满足不同场景下的氛围营造需求。')
    add_paragraph(doc, '继电器控制：包括通风扇、水泵、雾化器和安防总闸四路继电器。继电器采用高电平导通设计，安防总闸具有特殊的安全语义：高电平导通表示正常供电，低电平切断表示火灾熔断。')
    add_paragraph(doc, '舵机控制：智能遮雨棚采用舵机控制，支持 0°（收回）和 90°（展开）两个位置。舵机通过 LEDC PWM 驱动，精度为 1°，确保遮雨棚动作的平稳与可靠。')
    
    add_heading(doc, '6.2  成语意境场景系统', level=2)
    add_paragraph(doc, '系统创新性地设计了 7 种成语意境场景，将中华传统文化意象与现代智能控制深度融合，每种场景描绘一幅经典庭院意境，让用户一句成语即可沉浸式切换庭院氛围：')
    
    add_table(doc, ['成语', '英文键', '意境描述', '灯光配置'], [
        ['风和日丽', 'normal', '风和日暖，庭院悠然', '暖白(255,210,150) 亮度40 常亮'],
        ['月白风清', 'night', '月华如水，清风徐来', '月白(190,200,235) 亮度15 常亮'],
        ['清风徐来', 'hot', '水波不兴，清凉消暑', '冰蓝(70,185,250) 亮度45 呼吸'],
        ['闲庭信步', 'relax', '绿意盎然，信步闲游', '柔绿(65,210,95) 亮度28 呼吸'],
        ['烟雨朦胧', 'rain', '烟雨楼台，朦胧如画', '烟蓝(50,85,200) 亮度25 呼吸'],
        ['安全警戒', 'safe', '全开高亮，安全第一', '纯白(255,255,255) 亮度100 闪烁'],
        ['万籁俱寂', 'eco', '人静更深，万籁无声', '全部关闭']
    ])
    
    add_paragraph(doc, '场景切换时，系统会同时控制三路灯光的开关状态和灯带的 RGB 颜色、亮度、效果，实现一键切换的沉浸式体验。用户可以通过语音命令或 MCP 工具切换场景，例如说出"月白风清"，庭院即刻浸入月光如水的宁静氛围。')
    
    add_image_placeholder(doc, 'Web控制面板截图', '1200x800像素', '智慧庭院Web控制界面')
    
    add_heading(doc, '6.3  自动模式', level=2)
    add_paragraph(doc, '自动模式采用全局自动与单设备自动两层架构，实现了精细化的自动控制策略：')
    add_paragraph(doc, '全局自动模式：auto_mode 字段控制，开启后所有设备的自动接管生效，实现庭院环境的全局智能管理。')
    add_paragraph(doc, '单设备自动接管：auto_fan、auto_atomizer、auto_watering、auto_rain_shelter 四个字段分别控制通风扇、雾化器、水泵、遮雨棚的自动接管。手动控制某设备后，该设备的自动接管自动关闭，防止自动逻辑抢回控制权，充分尊重用户意图。')
    
    add_heading(doc, '6.4  传感器联动', level=2)
    add_paragraph(doc, '系统实现了多种传感器联动控制，构建了"感知—决策—执行"的智能闭环：')
    add_paragraph(doc, '（1）土壤湿度→水泵：土壤湿度低于 45% 时自动启动水泵，达到 60% 时停止，实现精准灌溉。')
    add_paragraph(doc, '（2）温湿度→风扇/雾化器：温度超过 32°C 或湿度超过 80% 时启动风扇，空气湿度低于 35% 时启动雾化器，智能调节庭院微气候。')
    add_paragraph(doc, '（3）雨滴→遮雨棚：雨滴百分比超过 60% 时自动展开遮雨棚到 90°，同时切换灯带到烟雨朦胧场景；雨停后自动收回遮雨棚并恢复风和日丽场景，实现天气感知的无缝自动化。')
    
    add_image_placeholder(doc, '传感器数据曲线图', '1000x600像素', '温湿度、土壤湿度等传感器数据变化曲线')
    
    add_heading(doc, '6.5  火灾安全闭环', level=2)
    add_paragraph(doc, '火灾检测采用 ADC 电压阈值轮询机制，实现了从感知到响应的全自动安全闭环，具体流程如下：')
    add_paragraph(doc, '（1）从机每 500ms 采样一次火焰传感器 ADC 值，电压下降表示火焰靠近，百分比上升。')
    add_paragraph(doc, '（2）fire_percent > 60% 连续 3 次（1.5 秒）判定为火灾确认，触发安全响应。')
    add_paragraph(doc, '（3）安全响应：安防总闸切断（GPIO17=0）、水泵/雾化器关闭、三路照明打开、灯带红色警示闪烁。')
    add_paragraph(doc, '（4）通过 MQTT 发布紧急事件，主机收到后进行 TTS 播报和蜂鸣器报警，实现多端协同告警。')
    add_paragraph(doc, '火灾自动解除：火焰移开后，电压回升，fire_percent <= 60% 持续 15 秒，自动恢复安防总闸导通和正常灯效。这种"检测—确认—响应—解除"的完整闭环，体现了系统在安全场景下的自治能力。')
    
    add_heading(doc, '6.6  能耗追踪系统', level=2)
    add_paragraph(doc, '系统创新性地实现了基于软件的能耗追踪功能，无需额外硬件传感器即可实时监测各设备用电情况：')
    add_paragraph(doc, '（1）采用额定功率×运行时间积分算法，精确计算各设备累计用电量（kWh）。')
    add_paragraph(doc, '（2）数据持久化存储于 NVS 非易失性存储器，断电不丢失。')
    add_paragraph(doc, '（3）支持设备状态变化时实时上报、每 2 小时兜底上报、MCU 启动时上报三种机制。')
    add_paragraph(doc, '（4）通过 MQTT 协议以 JSON 格式上报能耗数据，支持 App 端实时查看和历史统计。')
    
    doc.add_page_break()
    
    # 第七章 AI语音交互与MCP工具集成
    add_heading(doc, '第七章  AI 语音交互与 MCP 工具集成', level=1)
    
    add_heading(doc, '7.1  自研语音交互协议设计', level=2)
    add_paragraph(doc, '系统自研了完整的 AI 语音交互协议栈，实现从语音采集到智能响应的全链路闭环：')
    add_paragraph(doc, '（1）唤醒词检测：音频服务持续监听麦克风数据，当检测到唤醒词时触发唤醒事件，开启语音交互流程。')
    add_paragraph(doc, '（2）ASR 语音识别：唤醒后，系统将麦克风音频数据通过 MQTT 发送到云端进行语音识别，将自然语言转化为文本指令。')
    add_paragraph(doc, '（3）LLM 推理：云端对识别文本进行大语言模型推理，生成回复文本和 MCP 工具调用指令，实现"说即达意"的智能体验。')
    add_paragraph(doc, '（4）TTS 语音合成：回复文本通过 TTS 合成语音，通过 MQTT 返回设备进行播放，完成自然对话体验。')
    add_paragraph(doc, '系统支持三种监听模式：AutoStop（自动停止）、ManualStop（手动停止）和 Realtime（实时），根据 AEC 配置自动选择，灵活适配不同交互场景。')
    
    add_heading(doc, '7.2  MCP Server 架构', level=2)
    add_paragraph(doc, 'MCP（Model Context Protocol）Server 是大语言模型与物理世界的标准化桥梁。系统中的 McpServer 采用单例模式，提供以下核心能力：')
    add_paragraph(doc, '（1）工具注册：通过 AddTool 方法注册工具，每个工具包含名称、描述、参数列表和处理函数，实现插件化的功能扩展。')
    add_paragraph(doc, '（2）参数解析：支持字符串、整数、布尔值等参数类型，提供 PropertyList 进行参数验证，确保调用安全性。')
    add_paragraph(doc, '（3）结果返回：处理函数返回 cJSON 对象，McpServer 自动序列化为 JSON 字符串发送到云端，实现结构化的结果反馈。')
    
    add_heading(doc, '7.3  MCP 工具实现', level=2)
    add_paragraph(doc, '系统注册了 11 个 MCP 工具，全面覆盖庭院控制、状态查询、场景管理与安全响应等功能域：')
    
    add_table(doc, ['工具名称', '功能', '参数'], [
        ['self.garden.control', '统一控制庭院设备', 'target, action'],
        ['self.garden.set_strip_rgb', '设置灯带 RGB 颜色', 'red, green, blue, brightness, effect'],
        ['self.garden.set_ambience_scene', '设置意境场景', 'scene'],
        ['self.garden.calibrate_strip', '校准灯带色序', '无'],
        ['self.garden.emergency', '触发火灾紧急响应', '无'],
        ['self.garden.fire_reset', '解除火灾警报', '无'],
        ['self.garden.get_status', '获取设备状态', '无'],
        ['self.garden.get_sensors', '获取传感器数据', '无'],
        ['self.garden.set_auto_mode', '设置全局自动模式', 'enable'],
        ['self.garden.set_device_auto', '设置单设备自动接管', 'target, enable'],
        ['self.garden.all', '一键控制全部设备', 'scope, action']
    ])
    
    add_heading(doc, '7.4  对话实时转发', level=2)
    add_paragraph(doc, '系统实现了 AI 对话的实时转发功能，将用户的语音识别结果（ASR）和 AI 回复文本（TTS）通过 MQTT 发布到 xiaozhi/garden/chat Topic。消息格式为 JSON，包含角色（user/assistant）、文本内容与时间戳。App 端只需订阅该 Topic 即可实时查看用户与 AI 的对话内容，实现多端对话同步，让庭院智能交互透明可控。')
    
    add_heading(doc, '7.5  自然语言理解优化', level=2)
    add_paragraph(doc, '系统在自然语言理解方面进行了以下优化：')
    add_paragraph(doc, '（1）意图识别优化：通过 MCP 工具的描述和参数设计，引导大语言模型准确理解用户意图，减少误识别。')
    add_paragraph(doc, '（2）上下文维护：系统维护对话上下文，支持多轮对话，用户可以在对话中引用之前的设备状态。')
    add_paragraph(doc, '（3）容错处理：对于模糊或不完整的指令，系统能够通过上下文推断用户意图，并请求确认。')
    add_paragraph(doc, '（4）成语场景理解：系统能够理解成语意境描述，自动映射到对应的场景配置。')
    
    doc.add_page_break()
    
    # 第八章 App端接口设计
    add_heading(doc, '第八章  App 端接口设计', level=1)
    
    add_heading(doc, '8.1  MQTT 接口规范', level=2)
    add_paragraph(doc, 'App 端作为第二主机，可以通过 MQTT 直接控制庭院设备，无需经过小智主机，实现多通道平行控制。主要接口包括：')
    
    add_table(doc, ['订阅 Topic', '格式', '频率', '用途'], [
        ['xiaozhi/iot/sensor/+', 'JSON', '5 秒', '传感器原始数据'],
        ['xiaozhi/iot/heartbeat/+', '二进制', '10 秒', '从机在线心跳'],
        ['xiaozhi/iot/emergency/+', 'JSON', '事件触发', '火灾/雨天告警'],
        ['xiaozhi/garden/status', 'JSON', '5 秒', '设备状态（需主机在线）'],
        ['xiaozhi/garden/sensors', 'JSON', '5 秒', '传感器聚合（需主机在线）'],
        ['xiaozhi/garden/chat', 'JSON', '实时', 'AI 对话文本']
    ])
    
    add_heading(doc, '8.2  对话同步机制', level=2)
    add_paragraph(doc, 'App 端通过订阅 xiaozhi/garden/chat Topic 实现对话同步。该 Topic 由主机在收到 ASR 和 TTS 消息时自动发布，包含用户语音识别结果和 AI 回复文本。App 端可以实现以下功能：实时显示用户与 AI 的对话内容、记录对话历史、根据对话内容触发本地操作，让庭院智能交互在手机端也透明可见。')
    
    add_heading(doc, '8.3  App 端功能模块', level=2)
    add_paragraph(doc, 'App 端规划了以下功能模块，涵盖庭院智能管理的全场景体验：')
    add_paragraph(doc, '（1）用户界面设计：简洁直观的庭院控制台，一目了然的设备状态与环境数据。')
    add_paragraph(doc, '（2）设备控制界面：支持灯光、继电器、舵机等设备的远程控制与状态监控。')
    add_paragraph(doc, '（3）传感器数据展示：实时显示土壤湿度、温湿度、雨滴等环境数据，支持历史曲线查看。')
    add_paragraph(doc, '（4）场景切换界面：一键切换成语意境场景，沉浸式体验庭院氛围变化。')
    add_paragraph(doc, '（5）对话历史记录：查看与 AI 的历史对话，回溯智能交互过程。')
    add_paragraph(doc, '（6）推送通知机制：火灾、降雨等紧急事件实时推送，确保用户不错过任何重要信息。')
    
    add_image_placeholder(doc, 'App界面原型图', '800x1200像素', '移动端App界面设计稿')
    
    add_heading(doc, '8.4  数据可视化设计', level=2)
    add_paragraph(doc, 'App 端的数据可视化设计遵循以下原则：')
    add_paragraph(doc, '（1）实时数据仪表盘：采用卡片式布局，展示温湿度、土壤湿度、雨滴等关键指标，支持颜色编码（绿色=正常，黄色=警告，红色=异常）。')
    add_paragraph(doc, '（2）历史趋势图表：支持 24 小时、7 天、30 天的时间范围选择，展示传感器数据的变化趋势。')
    add_paragraph(doc, '（3）设备状态地图：以庭院平面图为基础，直观展示各设备的位置和运行状态。')
    add_paragraph(doc, '（4）能耗统计报表：以饼图和柱状图形式展示各设备的能耗占比和历史趋势。')
    
    doc.add_page_break()
    
    # 第九章 系统测试与总结
    add_heading(doc, '第九章  系统测试与总结', level=1)
    
    add_heading(doc, '9.1  功能测试', level=2)
    add_paragraph(doc, '系统功能测试全面覆盖了语音控制、自动模式、火灾响应与场景切换四大核心场景：')
    add_paragraph(doc, '语音控制测试：测试"打开庭院灯""关闭通风扇"等基本控制命令；测试成语意境场景切换："风和日丽""月白风清"等；测试复合命令："打开自动模式""全部关闭"等。所有测试均获得正确响应。')
    add_paragraph(doc, '自动模式测试：模拟土壤干燥：水泵自动启动，湿润后自动停止；模拟下雨：遮雨棚自动展开，雨停后自动收回；模拟高温：风扇和雾化器自动启动，温度回落后自动关闭。所有联动逻辑均符合预期。')
    add_paragraph(doc, '火灾响应测试：模拟火焰靠近：安防总闸切断，灯带红色警示，主机播报；火焰移开后：自动恢复总闸导通和正常灯效；手动解除火灾警报：通过语音命令或 MCP 工具。全部测试场景均通过验证。')
    add_paragraph(doc, '场景切换测试：测试 7 种成语意境场景的灯光效果；测试场景切换不影响自动模式的设备控制；测试安全警戒场景强制关断危险设备。场景切换流畅，与自动模式互不干扰。')
    
    add_image_placeholder(doc, '测试结果截图', '1200x800像素', '功能测试验证结果截图')
    
    add_heading(doc, '9.2  性能测试', level=2)
    add_paragraph(doc, '系统主要性能指标如下，充分体现了平台的实时性与可靠性：')
    
    add_table(doc, ['指标', '测试结果', '说明'], [
        ['MQTT 消息延迟', '< 100ms', '局域网内消息传输延迟'],
        ['传感器采样周期', '500ms', 'ADC 传感器采样间隔'],
        ['心跳间隔', '10 秒', '从机心跳发送间隔'],
        ['设备超时阈值', '60 秒', '设备离线判定阈值'],
        ['火灾检测响应时间', '1.5 秒', '连续 3 次超阈值判定'],
        ['火灾解除响应时间', '15 秒', '连续低于阈值时间']
    ])
    
    add_heading(doc, '9.3  可靠性测试', level=2)
    add_paragraph(doc, '系统可靠性测试主要验证以下方面：')
    add_paragraph(doc, '（1）网络断连恢复：测试 WiFi 断连后系统的自动重连能力，验证 MQTT 断线重连机制的有效性。')
    add_paragraph(doc, '（2）设备热插拔：测试从机设备的即插即用能力，验证设备发现和心跳机制的稳定性。')
    add_paragraph(doc, '（3）长时间运行：进行 72 小时连续运行测试，验证系统的长期稳定性。')
    add_paragraph(doc, '（4）异常恢复：测试火灾、网络异常等异常情况下的系统恢复能力。')
    
    add_heading(doc, '9.4  总结与展望', level=2)
    add_paragraph(doc, '本文提出并实现了一种基于 ESP32-S3 的智慧庭院生态平台，创新性地采用主从协同架构、MQTT 统一总线与 MCP 工具集成，实现了大模型驱动的语音控制、自动模式、成语意境场景切换和火灾安全闭环等核心功能。系统具有以下突出特点：')
    add_paragraph(doc, '（1）架构优雅：主从分离设计，主机负责智能交互，从机负责实时执行，职责清晰、解耦彻底。')
    add_paragraph(doc, '（2）通信统一：MQTT 作为统一通信总线，支持主机、从机、App 端的多端互联，松耦合、高可靠。')
    add_paragraph(doc, '（3）控制灵活：支持语音控制、JSON 命令和二进制命令多种控制方式，覆盖不同交互场景。')
    add_paragraph(doc, '（4）安全可靠：火灾检测采用本地 ADC 轮询，不依赖网络，确保极端情况下的安全性。')
    add_paragraph(doc, '（5）扩展性强：MCP 工具架构支持灵活扩展，新增设备只需注册新的工具即可接入系统。')
    add_paragraph(doc, '未来工作方向包括：开发完整的 App 端，实现移动端控制与数据展示；增加光照、风速等更多传感器类型；实现基于机器学习的智能预测控制；支持多庭院设备的分布式管理；集成云端数据分析平台，提供长期数据存储与智能分析服务。')
    
    doc.add_page_break()
    
    # 参考文献
    add_heading(doc, '参考文献', level=1)
    
    references = [
        '[1] Espressif Systems. ESP-IDF Programming Guide v5.5.3 [EB/OL]. https://docs.espressif.com/projects/esp-idf/.',
        '[2] MQTT Version 3.1.1 Specification [EB/OL]. https://mqtt.org/mqtt-specification/.',
        '[3] FreeRTOS Reference Manual [EB/OL]. https://www.freertos.org/Documentation/RTOS_book.html.',
        '[4] Model Context Protocol Specification [EB/OL]. https://modelcontextprotocol.io/.',
        '[5] WS2812B Datasheet [EB/OL]. https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf.',
        '[6] MPU6050 Register Map and Descriptions [EB/OL]. https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf.',
        '[7] INMP441 Datasheet [EB/OL]. https://www.invensense.com/wp-content/uploads/2015/02/INMP441.pdf.',
        '[8] MAX98357A Datasheet [EB/OL]. https://datasheets.maximintegrated.com/en/ds/MAX98357A-MAX98357B.pdf.',
        '[9] SSD1306 Datasheet [EB/OL]. https://www.solomon-systech.com/product/ssd1306/.',
        '[10] DHT11 Datasheet [EB/OL]. https://www.aosong.com/products-21.html.'
    ]
    
    for ref in references:
        add_paragraph(doc, ref)
    
    doc.add_page_break()
    
    # 附录
    add_heading(doc, '附录 A  完整代码清单', level=1)
    add_paragraph(doc, '（此处可放置核心代码片段或代码仓库链接）')
    
    add_heading(doc, '附录 B  硬件原理图', level=1)
    add_paragraph(doc, '（此处可放置完整的硬件原理图）')
    
    add_heading(doc, '附录 C  测试数据表', level=1)
    add_paragraph(doc, '（此处可放置详细的测试数据记录）')
    
    # 保存文档
    doc.save(OUTPUT_PATH)
    print(f'论文已生成: {OUTPUT_PATH}')

if __name__ == '__main__':
    create_thesis()
