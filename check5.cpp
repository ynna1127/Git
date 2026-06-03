#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <string>
#include "serialPort/SerialPort.h"
#include "unitreeMotor/unitreeMotor.h"

std::map<std::string, std::vector<int>> port_motors_map = {
    {"/dev/ttyUSB0", {0, 1, 2}},
    {"/dev/ttyUSB1", {0, 1, 2}},
    {"/dev/ttyUSB2", {0, 1, 2}},
    {"/dev/ttyUSB3", {0, 1, 2}},
};

// ==============================================
// 【修改后的控制函数】只给2号电机加控制参数
// ==============================================
void querySingleMotor(SerialPort &serial, const std::string &port_name, int motor_id) {
    MotorCmd cmd;
    MotorData data;
    cmd.motorType = MotorType::GO_M8010_6;
    data.motorType = MotorType::GO_M8010_6;
    cmd.mode = 10; // 【关键修改】改成10，伺服模式
    cmd.id = motor_id;

    // 【核心修改】只给2号电机加控制参数，其他保持零力
    if (motor_id == 2) {
        cmd.kp = 50.0f; // 加大kp，确保电机有力气
        cmd.kd = 1.0f;
        cmd.q = 1.0f; // 让2号电机转到1弧度（约57度）的位置
    } else {
        // 0、1号电机保持零力
        cmd.kp = 0.0f;
        cmd.kd = 0.0f;
        cmd.q = 0.0f;
    }

    cmd.dq = 0.0f;
    cmd.tau = 0.0f;
    serial.sendRecv(&cmd, &data);

    // 输出和原来完全一样
    std::cout << std::setw(16) << port_name
              << std::setw(8) << motor_id
              << std::setw(12) << std::fixed << std::setprecision(2) << data.q
              << std::setw(12) << std::fixed << std::setprecision(2) << data.dq
              << std::setw(8) << (int)data.temp
              << std::setw(8) << (int)data.merror;
    if (data.motor_id != motor_id) {
        std::cout << "  ❌ 离线/无响应";
    } else if (data.merror != 0) {
        std::cout << "  ⚠️  故障";
    } else {
        std::cout << "  ✅ 正常";
    }
    std::cout << std::endl;
}

int main() {
    std::map<std::string, SerialPort*> serial_ports;
    int total_motors = 0;
    for (auto &pair : port_motors_map) {
        std::string port_name = pair.first;
        try {
            serial_ports[port_name] = new SerialPort(port_name);
            std::cout << "✅ 串口打开成功: " << port_name << " (电机数: " << pair.second.size() << ")" << std::endl;
            total_motors += pair.second.size();
        } catch (...) {
            std::cerr << "❌ 串口打开失败: " << port_name << std::endl;
            serial_ports[port_name] = nullptr;
        }
    }
    std::cout << "\n共 " << serial_ports.size() << " 个串口，" << total_motors << " 个电机" << std::endl;
    std::cout << "\n【测试】所有2号电机会自动转到1弧度位置，观察电机是否转动" << std::endl;

    while (true) {
        std::cout << "\n==================================================================================" << std::endl;
        std::cout << "所属串口" << std::setw(8) << "电机ID"
                  << std::setw(12) << "位置(rad)"
                  << std::setw(12) << "速度(rad/s)"
                  << std::setw(8) << "温度(℃)"
                  << std::setw(8) << "故障码"
                  << std::setw(10) << "状态" << std::endl;
        std::cout << "----------------------------------------------------------------------------------" << std::endl;
        for (auto &port_pair : port_motors_map) {
            std::string port_name = port_pair.first;
            SerialPort *serial = serial_ports[port_name];
            if (serial == nullptr) {
                for (int motor_id : port_pair.second) {
                    std::cout << std::setw(16) << port_name
                              << std::setw(8) << motor_id
                              << std::setw(12) << "-"
                              << std::setw(12) << "-"
                              << std::setw(8) << "-"
                              << std::setw(8) << "-"
                              << "  ❌ 串口未打开" << std::endl;
                }
                continue;
            }
            for (int motor_id : port_pair.second) {
                querySingleMotor(*serial, port_name, motor_id);
            }
        }
        std::cout << "==================================================================================" << std::endl;
        usleep(500000); // 0.5秒刷新一次，更快看到效果
    }

    for (auto &pair : serial_ports) {
        if (pair.second != nullptr) {
            delete pair.second;
        }
    }
    return 0;
}