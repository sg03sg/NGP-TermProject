#pragma once

#include <string>

// 문자열 인코딩 변환
// 네트워크 문자열(닉네임, 채팅)은 UTF-8, 콘솔 입출력은 Windows 코드 페이지(한국어는 949) 사용
// codePage: 949, CP_ACP(0, 시스템 기본) 같은 코드 페이지 번호
// 변환할 수 없는 바이트가 있으면 false

bool ToUtf8(const std::string& text, unsigned int codePage, std::string& out);
bool FromUtf8(const std::string& utf8, unsigned int codePage, std::string& out);