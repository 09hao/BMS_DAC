# MOVE BMS DAC

Firmware ESP32 đọc dữ liệu BMS BLE và chuyển đổi SOC sang tín hiệu DAC cho đồng hồ hiển thị pin xe điện.

---

# Giới thiệu

Dự án này sử dụng ESP32 để kết nối Bluetooth Low Energy (BLE) với BMS của khối pin xe điện.

ESP32 sẽ:

- Kết nối BLE tới BMS
- Nhận dữ liệu pin theo thời gian thực
- Phân tích SOC (State Of Charge)
- Chuyển SOC thành tín hiệu DAC analog
- Xuất tín hiệu DAC ra đồng hồ báo pin

# Tính năng

- Kết nối BLE với BMS
- Tự động reconnect khi mất kết nối
- Đọc SOC thời gian thực
- Xuất DAC bằng DAC nội của ESP32
- Hỗ trợ nhiều dòng xe
- Dễ thay đổi profile xe
- Kiến trúc firmware dạng thư viện
- Dễ mở rộng và thương mại hóa

---

# Phần cứng hỗ trợ

## Vi điều khiển

- ESP32 Dev Module

## BMS hỗ trợ

- DEL BMS
- Các BMS BLE sử dụng FFE0 / FFE1

## Ngõ ra

- GPIO25 DAC
---

# BLE Protocol

## Service UUID

```text
FFE0
