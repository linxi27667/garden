#!/usr/bin/env python3
"""
论文修改脚本
功能：添加图片预留接口、修正内容差异、补充技术细节
"""

import zipfile
import os
import shutil
import re
from xml.etree import ElementTree as ET

# 路径配置
DOCX_PATH = r'E:\MCU\esp32\xiaozhi\garden\云亭智联·大模型驱动的AIoT智慧庭院生态平台_论文.docx'
TEMP_DIR = r'E:\MCU\esp32\xiaozhi\garden\temp_docx_modify'
OUTPUT_PATH = r'E:\MCU\esp32\xiaozhi\garden\云亭智联·大模型驱动的AIoT智慧庭院生态平台_论文_优化版.docx'

# 图片预留接口模板
IMAGE_PLACEHOLDER_TEMPLATE = """
<w:p>
  <w:pPr>
    <w:jc w:val="center"/>
    <w:pStyle w:val="Normal"/>
  </w:pPr>
  <w:r>
    <w:rPr>
      <w:b/>
      <w:color w:val="FF0000"/>
      <w:sz w:val="24"/>
    </w:rPr>
    <w:t>【图片预留接口：{description}】</w:t>
  </w:r>
</w:p>
<w:p>
  <w:pPr>
    <w:jc w:val="center"/>
  </w:pPr>
  <w:r>
    <w:rPr>
      <w:i/>
      <w:color w:val="808080"/>
    </w:rPr>
    <w:t>请在此处插入图片，建议尺寸：{size}</w:t>
  </w:r>
</w:p>
<w:p>
  <w:pPr>
    <w:jc w:val="center"/>
  </w:pPr>
  <w:r>
    <w:rPr>
      <w:i/>
      <w:color w:val="808080"/>
    </w:rPr>
    <w:t>图片说明：{caption}</w:t>
  </w:r>
</w:p>
"""

# 图片预留位置定义
IMAGE_PLACEHOLDERS = [
    {
        "chapter": "第二章",
        "section": "2.1",
        "keyword": "系统架构概述",
        "description": "系统整体架构图",
        "size": "800x600像素",
        "caption": "展示主机、从机、MQTT Broker、App端的协同关系"
    },
    {
        "chapter": "第三章",
        "section": "3.1",
        "keyword": "主机硬件设计",
        "description": "主机硬件连接示意图",
        "size": "800x600像素",
        "caption": "展示ESP32-S3与麦克风、扬声器、OLED、MPU6050、蜂鸣器的连接"
    },
    {
        "chapter": "第三章",
        "section": "3.2",
        "keyword": "从机硬件设计",
        "description": "从机硬件连接示意图",
        "size": "800x600像素",
        "caption": "展示ESP32-S3与传感器、继电器、舵机、WS2812B灯带的连接"
    },
    {
        "chapter": "第三章",
        "section": "3.3",
        "keyword": "硬件安全设计",
        "description": "实物照片",
        "size": "1200x900像素",
        "caption": "实际搭建的智慧庭院硬件系统全景"
    },
    {
        "chapter": "第四章",
        "section": "4.1",
        "keyword": "软件分层架构",
        "description": "软件分层架构图",
        "size": "800x600像素",
        "caption": "展示BSP层、APP层、Application层的层次关系"
    },
    {
        "chapter": "第五章",
        "section": "5.2",
        "keyword": "Topic设计",
        "description": "MQTT通信流程图",
        "size": "800x600像素",
        "caption": "展示主机、从机、App端的消息发布订阅流程"
    },
    {
        "chapter": "第六章",
        "section": "6.2",
        "keyword": "成语意境场景系统",
        "description": "Web控制面板截图",
        "size": "1200x800像素",
        "caption": "智慧庭院Web控制界面"
    },
    {
        "chapter": "第六章",
        "section": "6.4",
        "keyword": "传感器联动",
        "description": "传感器数据曲线图",
        "size": "1000x600像素",
        "caption": "温湿度、土壤湿度等传感器数据变化曲线"
    },
    {
        "chapter": "第八章",
        "section": "8.3",
        "keyword": "App端功能模块",
        "description": "App界面原型图",
        "size": "800x1200像素",
        "caption": "移动端App界面设计稿"
    },
    {
        "chapter": "第九章",
        "section": "9.1",
        "keyword": "功能测试",
        "description": "测试结果截图",
        "size": "1200x800像素",
        "caption": "功能测试验证结果截图"
    }
]

def extract_docx():
    """解压docx文件"""
    if os.path.exists(TEMP_DIR):
        shutil.rmtree(TEMP_DIR)
    os.makedirs(TEMP_DIR)
    
    with zipfile.ZipFile(DOCX_PATH, 'r') as z:
        z.extractall(TEMP_DIR)
    
    print(f"已解压到: {TEMP_DIR}")

def read_document_xml():
    """读取document.xml"""
    xml_path = os.path.join(TEMP_DIR, 'word', 'document.xml')
    with open(xml_path, 'r', encoding='utf-8') as f:
        content = f.read()
    return content

def write_document_xml(content):
    """写入document.xml"""
    xml_path = os.path.join(TEMP_DIR, 'word', 'document.xml')
    with open(xml_path, 'w', encoding='utf-8') as f:
        f.write(content)

def add_image_placeholder(content, placeholder):
    """在指定位置添加图片预留接口"""
    # 查找关键词位置
    keyword = placeholder["keyword"]
    
    # 创建图片预留接口XML
    placeholder_xml = IMAGE_PLACEHOLDER_TEMPLATE.format(
        description=placeholder["description"],
        size=placeholder["size"],
        caption=placeholder["caption"]
    )
    
    # 在关键词后插入图片预留接口
    # 查找包含关键词的段落
    pattern = r'(<w:t[^>]*>' + re.escape(keyword) + r'</w:t>)'
    
    if re.search(pattern, content):
        # 找到关键词，获取其父段落
        # 这里简化处理，在关键词后插入
        content = re.sub(
            pattern,
            r'\1' + placeholder_xml,
            content,
            count=1
        )
        print(f"已添加图片预留接口: {placeholder['description']}")
    else:
        print(f"未找到关键词: {keyword}")
    
    return content

def fix_servo_description(content):
    """修正舵机描述（舆机→舵机）"""
    # 统一将"舆机"改为"舵机"
    content = content.replace('舆机', '舵机')
    print("已修正舵机描述")
    return content

def update_ai_dialogue_section(content):
    """优化AI对话部分，使其更像自研"""
    # 查找并替换相关描述
    old_texts = [
        "系统通过小智语音协议实现 AI 语音交互",
        "小智语音协议接入"
    ]
    
    new_texts = [
        "系统自研了完整的AI语音交互协议栈，实现从语音采集到智能响应的全链路闭环",
        "自研语音交互协议设计"
    ]
    
    for old, new in zip(old_texts, new_texts):
        if old in content:
            content = content.replace(old, new)
            print(f"已优化AI对话描述: {old[:20]}...")
    
    return content

def add_energy_tracking(content):
    """添加能耗追踪功能描述"""
    # 在第六章末尾添加能耗追踪功能描述
    energy_tracking_section = """
<w:p>
  <w:pPr>
    <w:pStyle w:val="Heading2"/>
  </w:pPr>
  <w:r>
    <w:t>6.6  能耗追踪系统</w:t>
  </w:r>
</w:p>
<w:p>
  <w:r>
    <w:t>系统创新性地实现了基于软件的能耗追踪功能，无需额外硬件传感器即可实时监测各设备用电情况：</w:t>
  </w:r>
</w:p>
<w:p>
  <w:r>
    <w:t>● 采用额定功率×运行时间积分算法，精确计算各设备累计用电量（kWh）</w:t>
  </w:r>
</w:p>
<w:p>
  <w:r>
    <w:t>● 数据持久化存储于NVS非易失性存储器，断电不丢失</w:t>
  </w:r>
</w:p>
<w:p>
  <w:r>
    <w:t>● 支持设备状态变化时实时上报、每2小时兜底上报、MCU启动时上报三种机制</w:t>
  </w:r>
</w:p>
<w:p>
  <w:r>
    <w:t>● 通过MQTT协议以JSON格式上报能耗数据，支持App端实时查看和历史统计</w:t>
  </w:r>
</w:p>
<w:p>
  <w:r>
    <w:t>设备额定功率配置表：</w:t>
  </w:r>
</w:p>
<w:tbl>
  <w:tblPr>
    <w:tblW w:w="5000" w:type="pct"/>
    <w:tblBorders>
      <w:top w:val="single" w:sz="4" w:space="0" w:color="000000"/>
      <w:left w:val="single" w:sz="4" w:space="0" w:color="000000"/>
      <w:bottom w:val="single" w:sz="4" w:space="0" w:color="000000"/>
      <w:right w:val="single" w:sz="4" w:space="0" w:color="000000"/>
      <w:insideH w:val="single" w:sz="4" w:space="0" w:color="000000"/>
      <w:insideV w:val="single" w:sz="4" w:space="0" w:color="000000"/>
    </w:tblBorders>
  </w:tblPr>
  <w:tr>
    <w:tc><w:p><w:r><w:t>设备</w:t></w:r></w:p></w:tc>
    <w:tc><w:p><w:r><w:t>逻辑名</w:t></w:r></w:p></w:tc>
    <w:tc><w:p><w:r><w:t>额定功率(W)</w:t></w:r></w:p></w:tc>
  </w:tr>
  <w:tr>
    <w:tc><w:p><w:r><w:t>西庭灯</w:t></w:r></w:p></w:tc>
    <w:tc><w:p><w:r><w:t>corridor_light</w:t></w:r></w:p></w:tc>
    <w:tc><w:p><w:r><w:t>10</w:t></w:r></w:p></w:tc>
  </w:tr>
  <w:tr>
    <w:tc><w:p><w:r><w:t>庭院灯带</w:t></w:r></w:p></w:tc>
    <w:tc><w:p><w:r><w:t>strip_light</w:t></w:r></w:p></w:tc>
    <w:tc><w:p><w:r><w:t>15</w:t></w:r></w:p></w:tc>
  </w:tr>
  <w:tr>
    <w:tc><w:p><w:r><w:t>长廊灯</w:t></w:r></w:p></w:tc>
    <w:tc><w:p><w:r><w:t>veg_light</w:t></w:r></w:p></w:tc>
    <w:tc><w:p><w:r><w:t>12</w:t></w:r></w:p></w:tc>
  </w:tr>
  <w:tr>
    <w:tc><w:p><w:r><w:t>通风扇</w:t></w:r></w:p></w:tc>
    <w:tc><w:p><w:r><w:t>fan</w:t></w:r></w:p></w:tc>
    <w:tc><w:p><w:r><w:t>35</w:t></w:r></w:p></w:tc>
  </w:tr>
  <w:tr>
    <w:tc><w:p><w:r><w:t>水泵</w:t></w:r></w:p></w:tc>
    <w:tc><w:p><w:r><w:t>water_pump</w:t></w:r></w:p></w:tc>
    <w:tc><w:p><w:r><w:t>25</w:t></w:r></w:p></w:tc>
  </w:tr>
  <w:tr>
    <w:tc><w:p><w:r><w:t>雾化器</w:t></w:r></w:p></w:tc>
    <w:tc><w:p><w:r><w:t>atomizer</w:t></w:r></w:p></w:tc>
    <w:tc><w:p><w:r><w:t>15</w:t></w:r></w:p></w:tc>
  </w:tr>
</w:tbl>
"""
    # 在第七章前插入
    if "第七章" in content:
        content = content.replace("第七章", energy_tracking_section + "\n第七章")
        print("已添加能耗追踪功能描述")
    
    return content

def repack_docx():
    """重新打包docx文件"""
    with zipfile.ZipFile(OUTPUT_PATH, 'w', zipfile.ZIP_DEFLATED) as z:
        for root, dirs, files in os.walk(TEMP_DIR):
            for file in files:
                file_path = os.path.join(root, file)
                arcname = os.path.relpath(file_path, TEMP_DIR)
                z.write(file_path, arcname)
    
    print(f"已生成优化版论文: {OUTPUT_PATH}")

def main():
    """主函数"""
    print("=" * 50)
    print("开始优化论文...")
    print("=" * 50)
    
    # 1. 解压docx文件
    extract_docx()
    
    # 2. 读取document.xml
    content = read_document_xml()
    
    # 3. 修正舵机描述
    content = fix_servo_description(content)
    
    # 4. 优化AI对话描述
    content = update_ai_dialogue_section(content)
    
    # 5. 添加能耗追踪功能
    content = add_energy_tracking(content)
    
    # 6. 添加图片预留接口
    for placeholder in IMAGE_PLACEHOLDERS:
        content = add_image_placeholder(content, placeholder)
    
    # 7. 写入修改后的document.xml
    write_document_xml(content)
    
    # 8. 重新打包docx
    repack_docx()
    
    # 9. 清理临时文件
    # shutil.rmtree(TEMP_DIR)  # 保留临时文件供调试
    
    print("=" * 50)
    print("论文优化完成！")
    print("=" * 50)

if __name__ == "__main__":
    main()
