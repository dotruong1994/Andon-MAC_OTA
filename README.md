Cách sử dụng code này: 
Vì chương trình này chạy OTA độc lập để fake MAC, nên mỗi còn sẽ có một IP riêng mặc định 

***********************

IT 10.101.103.201 --> 252   
// lựa chọn trong các IP này theo thứ tự từ lớn đến nhỏ   
// lưu ý: ko dùng từ 210 trở về 201 ( vì đã có máy khác dùng rồi )

SM 255.255.248.0
Gw 10.101.103.254
DNS: 172.21.130.1 - 2

***********************
Trường hợp code demo trong src này đã sử dụng 250 và gắn ở A8 <-----------
Dòng code số 23:    IPAddress local_IP(10, 101, 103, 250); // đổi IP này để cho mấy con fake MAC
Dòng code số 35:    #define FIRMWARE_VERSION "2.0"  // dòng này chủ yếu dùng cho OTA chính quy

Dòng code số 77:    uint8_t newMACAddress[] = {0x9C, 0x9C, 0x1F, 0xE3, 0xFE, 0x6C};    // đổi cái MAC này theo cái con ban đầu phải fake, trường hợp này là con bên A8

OK xong! GoodLuck!









