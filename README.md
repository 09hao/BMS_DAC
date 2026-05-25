# MOVE BMS DAC

Firmware ESP32 đọc dữ liệu BMS BLE và chuyển đổi SOC thành tín hiệu DAC analog cho đồng hồ báo pin xe điện.

---

# Chức năng chính

- Kết nối BLE tới BMS
- Đọc SOC thời gian thực
- Tự động dò số cell trong khối pin
- Đọc điện áp từng cell
- Tính độ lệch cell
- Cảnh báo / báo lỗi pin
- Xuất DAC GPIO25 cho đồng hồ xe
- Tự reconnect khi mất BLE
- Fail-safe khi mất dữ liệu BMS

---

# Nguyên lý hoạt động

```text
BMS BLE
   ↓
ESP32
   ↓
Đọc SOC + Cell Voltage
   ↓
Kiểm tra lệch cell
   ↓
Xuất DAC GPIO25
   ↓
Đồng hồ báo pin
```

---

# Phần cứng

## Vi điều khiển

- ESP32 Dev Module
- ESP32-WROOM-32

> Yêu cầu ESP32 có DAC nội.

## BMS hỗ trợ

- DEL BMS BLE
- Các BMS BLE dùng FFE0 / FFE1

## Ngõ ra

- GPIO25 DAC

---

# BLE Protocol

## Service UUID

```text
0000FFE0-0000-1000-8000-00805F9B34FB
```

## Characteristic UUID

```text
0000FFE1-0000-1000-8000-00805F9B34FB
```

## Login Command

```text
A5 0B 00 58 58 19 0A 1E 0E 28 0D
```

---

# Tính năng cell

Firmware tự:

- dò vùng dữ liệu cell trong packet BLE
- xác định số lượng cell
- đọc điện áp từng cell
- tính:
  - Cell Min
  - Cell Max
  - Cell Delta

---

# Báo lỗi pin

## Cảnh báo

```text
Cell Delta >= 80mV
```

→ giới hạn mức pin hiển thị

## Báo lỗi

```text
Cell Delta >= 150mV
```

hoặc:

```text
Cell Min <= 2800mV
```

→ ép DAC về mức pin yếu

---

# Fail-safe

## Mất BLE

```text
DAC → mức pin thấp
Reconnect BMS
```

## Timeout dữ liệu BMS

Nếu không có packet > 5 giây:

```text
DAC → mức pin thấp
```

---

# Hỗ trợ nhiều dòng xe

Firmware hỗ trợ nhiều profile xe:

- ISB
- ATN
- 007

Mỗi xe có:

- dacMin
- dacMax
- socIndex

riêng.

---

# Cấu trúc project

```text
BMS_DAC.ino
MoveBMSGauge.h
MoveBMSGauge.cpp
```

---

# Kết quả hiện tại

Firmware đã thực hiện được:

```text
BMS BLE
→ ESP32
→ đọc SOC
→ tự dò số cell
→ đọc điện áp từng cell
→ tính lệch cell
→ báo lỗi pin
→ xuất DAC
→ đồng hồ xe
```