#include <WiFi.h>
#include <Preferences.h>
#include <time.h>
#include "M5Cardputer.h"

#define MAX_APS       30
#define ROWS_PER_PAGE 5

struct APInfo {
    String ssid;
    int32_t rssi;
    uint8_t channel;
    wifi_auth_mode_t authmode;
};

APInfo apList[MAX_APS];
int apCount = 0;

enum AppState {
    STATE_LIST,
    STATE_DETAIL,
    STATE_INPUT,
    STATE_SETTINGS
};
AppState currentState = STATE_LIST;

enum Language { LANG_CN, LANG_EN };
Language currentLang = LANG_EN;

int settingsSubMenu = 0;
int settingsSelected = 0;
int langSelected = 0;

int selectedIndex = 0;
int topIndex = 0;

int selectedButton = 0;
String statusMsg = "";

String passwordInput = "";
bool isConnecting = false;

bool timeSynced = false;
unsigned long lastTimeUpdate = 0;

int lastBatteryLevel = -1;
unsigned long lastBatteryCheck = 0;

Preferences prefs;

void loadLanguage() {
    prefs.begin("wifi_cfg", true);
    currentLang = (Language)prefs.getInt("lang", LANG_EN);
    prefs.end();
}
void saveLanguage(Language lang) {
    prefs.begin("wifi_cfg", false);
    prefs.putInt("lang", (int)lang);
    prefs.end();
}

const char* T(const char* cn, const char* en) {
    return (currentLang == LANG_CN) ? cn : en;
}

const char* rssiToLabel(int32_t rssi) {
    if (currentLang == LANG_CN) {
        if (rssi >= -50) return "极强";
        if (rssi >= -60) return "强";
        if (rssi >= -70) return "中";
        if (rssi >= -80) return "弱";
        return "极弱";
    } else {
        if (rssi >= -50) return "Excellent";
        if (rssi >= -60) return "Good";
        if (rssi >= -70) return "Fair";
        if (rssi >= -80) return "Weak";
        return "Very Weak";
    }
}

int rssiToBars(int32_t rssi) {
    if (rssi >= -50) return 5;
    if (rssi >= -60) return 4;
    if (rssi >= -70) return 3;
    if (rssi >= -80) return 2;
    if (rssi >= -90) return 1;
    return 0;
}

uint16_t rssiToColor(int32_t rssi) {
    if (rssi >= -60) return TFT_GREEN;
    if (rssi >= -75) return TFT_YELLOW;
    return TFT_RED;
}

String authToStr(wifi_auth_mode_t mode) {
    switch (mode) {
        case WIFI_AUTH_OPEN:            return T("开放", "Open");
        case WIFI_AUTH_WEP:             return "WEP";
        case WIFI_AUTH_WPA_PSK:         return "WPA";
        case WIFI_AUTH_WPA2_PSK:        return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-E";
        case WIFI_AUTH_WPA3_PSK:        return "WPA3";
        default:                        return T("其他", "Other");
    }
}

void drawSignalBars(int x, int y, int32_t rssi) {
    int bars = rssiToBars(rssi);
    uint16_t color = rssiToColor(rssi);
    int barW = 3, gap = 1, maxH = 10;
    for (int i = 0; i < 5; i++) {
        int h = maxH - (4 - i) * 2;
        int bx = x + i * (barW + gap);
        int by = y + maxH - h;
        if (i < bars) M5.Display.fillRect(bx, by, barW, h, color);
        else M5.Display.drawRect(bx, by, barW, h, TFT_DARKGREY);
    }
}

void drawBatteryIcon(int x, int y, int level, bool charging) {
    int w = 18, h = 9;
    M5.Display.drawRect(x, y, w, h, TFT_WHITE);
    M5.Display.fillRect(x + w, y + 2, 2, 5, TFT_WHITE);

    if (level < 0) level = 0;
    if (level > 100) level = 100;
    int fillW = (w - 4) * level / 100;
    uint16_t color;
    if (level > 50) color = TFT_GREEN;
    else if (level > 20) color = TFT_YELLOW;
    else color = TFT_RED;
    if (fillW > 0) M5.Display.fillRect(x + 2, y + 2, fillW, h - 4, color);

    if (charging) {
        int cx = x + w / 2;
        int cy = y + h / 2;
        M5.Display.drawLine(cx, y + 1, cx - 3, cy, TFT_WHITE);
        M5.Display.drawLine(cx - 3, cy, cx + 2, cy, TFT_WHITE);
        M5.Display.drawLine(cx + 2, cy, cx, y + h - 1, TFT_WHITE);
    }
}

float easeOutBack(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return 1.0f + c3 * powf(t - 1.0f, 3) + c1 * powf(t - 1.0f, 2);
}

float easeOutCubic(float t) {
    return 1.0f - powf(1.0f - t, 3);
}

uint16_t lerpColor(uint16_t c1, uint16_t c2, float t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    int r1 = (c1 >> 11) & 0x1F;
    int g1 = (c1 >> 5) & 0x3F;
    int b1 = c1 & 0x1F;
    int r2 = (c2 >> 11) & 0x1F;
    int g2 = (c2 >> 5) & 0x3F;
    int b2 = c2 & 0x1F;
    int r = r1 + (r2 - r1) * t;
    int g = g1 + (g2 - g1) * t;
    int b = b1 + (b2 - b1) * t;
    return (r << 11) | (g << 5) | b;
}

void drawWifiIconScaled(int cx, int cy, float scale, uint16_t color) {
    if (scale <= 0.05f) return;

    int r_o1 = (int)(32 * scale);
    int r_i1 = (int)(26 * scale);
    int r_o2 = (int)(23 * scale);
    int r_i2 = (int)(17 * scale);
    int r_o3 = (int)(14 * scale);
    int r_i3 = (int)(8 * scale);
    int dotR = (int)(3 * scale);

    if (r_o1 > r_i1 && r_i1 > 0) M5.Display.drawArc(cx, cy, r_o1, r_i1, 225, 315, color);
    if (r_o2 > r_i2 && r_i2 > 0) M5.Display.drawArc(cx, cy, r_o2, r_i2, 225, 315, color);
    if (r_o3 > r_i3 && r_i3 > 0) M5.Display.drawArc(cx, cy, r_o3, r_i3, 225, 315, color);
    if (dotR > 0) M5.Display.fillCircle(cx, cy, dotR, color);
}

void bootFeedback() {
    M5.Speaker.begin();
    M5.Speaker.setVolume(80);

    M5.Led.setBrightness(128);
    M5.Led.setColor(0, 255, 255, 255);
    M5.Led.display();

    M5.Speaker.tone(1200, 100);

    delay(100);

    M5.Led.setColor(0, 0, 0, 0);
    M5.Led.display();
    M5.Speaker.stop();
}

void drawBootScreen() {
    const int cx = 120;
    const int iconCY = 62;

    const int text1Y = 82;
    const int text2Y = 100;

    const unsigned long ICON_START = 0;
    const unsigned long ICON_END = 800;
    const unsigned long TEXT1_START = 500;
    const unsigned long TEXT1_END = 1100;
    const unsigned long TEXT2_START = 700;
    const unsigned long TEXT2_END = 1300;
    const unsigned long ANIM_END = 1600;

    unsigned long startMs = millis();

    while (true) {
        unsigned long now = millis() - startMs;

        float iconT = 0;
        if (now >= ICON_START) {
            iconT = (float)(now - ICON_START) / (ICON_END - ICON_START);
            if (iconT > 1) iconT = 1;
        }
        float text1T = 0;
        if (now >= TEXT1_START) {
            text1T = (float)(now - TEXT1_START) / (TEXT1_END - TEXT1_START);
            if (text1T > 1) text1T = 1;
        }
        float text2T = 0;
        if (now >= TEXT2_START) {
            text2T = (float)(now - TEXT2_START) / (TEXT2_END - TEXT2_START);
            if (text2T > 1) text2T = 1;
        }

        float iconE = easeOutBack(iconT);
        float text1E = easeOutCubic(text1T);
        float text2E = easeOutCubic(text2T);

        M5.Display.fillScreen(TFT_BLACK);

        drawWifiIconScaled(cx, iconCY, iconE, TFT_WHITE);

        M5.Display.setFont(&fonts::efontCN_12);
        int t1W = M5.Display.textWidth("Created by DeepSeek");
        int t1X = (240 - t1W) / 2;
        int t1Y = text1Y + (int)(18 * (1 - text1E));
        uint16_t c1 = lerpColor(TFT_BLACK, TFT_WHITE, text1E);
        M5.Display.setTextColor(c1, TFT_BLACK);
        M5.Display.setCursor(t1X, t1Y);
        M5.Display.print("Created by DeepSeek");

        int t2W = M5.Display.textWidth("WiFi Scanner");
        int t2X = (240 - t2W) / 2;
        int t2Y = text2Y + (int)(18 * (1 - text2E));
        uint16_t c2 = lerpColor(TFT_BLACK, TFT_WHITE, text2E);
        M5.Display.setTextColor(c2, TFT_BLACK);
        M5.Display.setCursor(t2X, t2Y);
        M5.Display.print("WiFi Scanner");

        if (now >= ANIM_END) break;

        delay(16);
    }

    delay(600);
}

void drawTopBar() {
    M5.Display.fillRect(0, 0, 240, 22, TFT_NAVY);
    M5.Display.setTextColor(TFT_WHITE, TFT_NAVY);
    M5.Display.setFont(&fonts::efontCN_12);

    M5.Display.setCursor(2, 4);
    M5.Display.printf("WiFi %d", apCount);

    if (timeSynced) {
        struct tm ti;
        if (getLocalTime(&ti)) {
            char buf[10];
            strftime(buf, sizeof(buf), "%H:%M:%S", &ti);
            M5.Display.setCursor(58, 4);
            M5.Display.setTextColor(TFT_WHITE, TFT_NAVY);
            M5.Display.print(buf);
        }
    }

    int level = M5.Power.getBatteryLevel();
    bool charging = M5.Power.isCharging();
    if (level >= 0) {
        M5.Display.setCursor(118, 4);
        M5.Display.setTextColor(TFT_WHITE, TFT_NAVY);
        M5.Display.printf("%d%%", level);
        drawBatteryIcon(146, 7, level, charging);
    }

    int wifiIconX = 205;
    if (WiFi.status() == WL_CONNECTED) {
        M5.Display.drawArc(wifiIconX, 15, 8, 6, 225, 315, TFT_GREEN);
        M5.Display.drawArc(wifiIconX, 15, 5, 3, 225, 315, TFT_GREEN);
        M5.Display.fillCircle(wifiIconX, 15, 1.5, TFT_GREEN);
    } else {
        M5.Display.drawLine(wifiIconX - 4, 9, wifiIconX + 4, 18, TFT_RED);
        M5.Display.drawLine(wifiIconX + 4, 9, wifiIconX - 4, 18, TFT_RED);
    }
}

void drawListPage();
void drawDetailPage();
void drawInputPage();
void drawSettingsPage();

void doScan() {
    statusMsg = T("扫描中...", "Scanning...");
    drawListPage();
    delay(150);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(80);

    int n = WiFi.scanNetworks(false, true);
    apCount = 0;
    if (n <= 0) {
        statusMsg = T("未发现 WiFi", "No WiFi found");
        selectedIndex = 0; topIndex = 0;
        return;
    }
    int count = (n > MAX_APS) ? MAX_APS : n;
    for (int i = 0; i < count; i++) {
        apList[i].ssid = WiFi.SSID(i);
        apList[i].rssi = WiFi.RSSI(i);
        apList[i].channel = WiFi.channel(i);
        apList[i].authmode = WiFi.encryptionType(i);
    }
    apCount = count;
    for (int i = 0; i < apCount - 1; i++)
        for (int j = 0; j < apCount - 1 - i; j++)
            if (apList[j].rssi < apList[j + 1].rssi) {
                APInfo t = apList[j]; apList[j] = apList[j + 1]; apList[j + 1] = t;
            }
    selectedIndex = 0; topIndex = 0;
    statusMsg = "";
    WiFi.scanDelete();
}

void drawListPage() {
    M5.Display.fillScreen(TFT_BLACK);
    drawTopBar();
    M5.Display.setFont(&fonts::efontCN_12);

    if (apCount == 0) {
        M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Display.setCursor(10, 55);
        M5.Display.print(T("暂无数据，按空格重扫", "No data, press SPACE to rescan"));
        return;
    }

    if (selectedIndex < topIndex) topIndex = selectedIndex;
    else if (selectedIndex >= topIndex + ROWS_PER_PAGE) topIndex = selectedIndex - ROWS_PER_PAGE + 1;

    int y = 26;
    for (int i = 0; i < ROWS_PER_PAGE; i++) {
        int idx = topIndex + i;
        if (idx >= apCount) break;
        bool sel = (idx == selectedIndex);
        uint16_t bg = sel ? TFT_BLUE : TFT_BLACK;
        if (sel) M5.Display.fillRect(0, y, 240, 18, bg);

        M5.Display.setTextColor(sel ? TFT_WHITE : TFT_CYAN, bg);
        M5.Display.setCursor(2, y + 2);
        M5.Display.printf("%2d", idx + 1);

        M5.Display.setTextColor(sel ? TFT_WHITE : TFT_LIGHTGREY, bg);
        M5.Display.setCursor(24, y + 2);
        String name = apList[idx].ssid;
        if (name.length() == 0) name = T("(隐藏)", "(Hidden)");
        if (name.length() > 10) name = name.substring(0, 10) + "..";
        M5.Display.print(name);

        M5.Display.setTextColor(sel ? TFT_WHITE : TFT_LIGHTGREY, bg);
        M5.Display.setCursor(130, y + 2);
        M5.Display.printf("CH%02d", apList[idx].channel);

        M5.Display.setTextColor(rssiToColor(apList[idx].rssi), bg);
        M5.Display.setCursor(168, y + 2);
        M5.Display.printf("%4d", apList[idx].rssi);

        drawSignalBars(208, y + 3, apList[idx].rssi);
        y += 18;
    }

    M5.Display.fillRect(0, 122, 240, 13, TFT_NAVY);
    M5.Display.setTextColor(TFT_WHITE, TFT_NAVY);
    M5.Display.setCursor(4, 124);
    if (statusMsg != "") M5.Display.print(statusMsg);
    else M5.Display.print("H:HELP");
}

void drawDetailPage() {
    M5.Display.fillScreen(TFT_BLACK);
    drawTopBar();
    M5.Display.setFont(&fonts::efontCN_12);

    if (selectedIndex >= apCount) { currentState = STATE_LIST; return; }
    APInfo& ap = apList[selectedIndex];

    M5.Display.fillRect(0, 22, 240, 20, TFT_DARKGREY);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREY);
    M5.Display.setCursor(6, 24);
    String nm = ap.ssid.length() == 0 ? T("(隐藏网络)", "(Hidden)") : ap.ssid;
    M5.Display.print(nm);

    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(10, 48);
    M5.Display.printf("%s: %d dBm (%s)", T("信号", "Signal"), ap.rssi, rssiToLabel(ap.rssi));
    M5.Display.setCursor(10, 63);
    M5.Display.printf("%s: %d", T("信道", "Channel"), ap.channel);
    M5.Display.setCursor(10, 78);
    M5.Display.printf("%s: %s", T("加密", "Auth"), authToStr(ap.authmode).c_str());

    int btnY = 100, btnW = 70, btnH = 24;
    uint16_t c1 = (selectedButton == 0) ? TFT_GREEN : TFT_DARKGREY;
    M5.Display.fillRect(10, btnY, btnW, btnH, c1);
    M5.Display.setTextColor(TFT_BLACK, c1);
    M5.Display.setCursor(33, btnY + 5); M5.Display.print(T("连接", "Connect"));

    uint16_t c2 = (selectedButton == 1) ? TFT_ORANGE : TFT_DARKGREY;
    M5.Display.fillRect(85, btnY, btnW, btnH, c2);
    M5.Display.setTextColor(TFT_BLACK, c2);
    M5.Display.setCursor(108, btnY + 5); M5.Display.print(T("保存", "Save"));

    uint16_t c3 = (selectedButton == 2) ? TFT_RED : TFT_DARKGREY;
    M5.Display.fillRect(160, btnY, btnW, btnH, c3);
    M5.Display.setTextColor(TFT_WHITE, c3);
    M5.Display.setCursor(183, btnY + 5); M5.Display.print(T("返回", "Back"));

    M5.Display.fillRect(0, 122, 240, 13, TFT_NAVY);
    M5.Display.setTextColor(TFT_WHITE, TFT_NAVY);
    M5.Display.setCursor(4, 124);
    if (statusMsg != "") M5.Display.print(statusMsg);
    else M5.Display.print(T("↑↓切换 回车确认 Fn+`返回", "Arrows:Switch ENTER:OK Fn+`:Back"));
}

void drawInputPage() {
    M5.Display.fillScreen(TFT_BLACK);
    drawTopBar();
    M5.Display.setFont(&fonts::efontCN_12);

    M5.Display.fillRect(0, 22, 240, 20, TFT_DARKGREY);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREY);
    M5.Display.setCursor(6, 24);
    M5.Display.print(T("请输入 WiFi 密码", "Enter WiFi Password"));

    M5.Display.drawRect(10, 50, 220, 30, TFT_WHITE);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setCursor(15, 58);
    String disp = passwordInput;
    if (disp.length() > 20) disp = "..." + disp.substring(disp.length() - 17);
    M5.Display.print(disp);
    if ((millis() / 500) % 2 == 0) M5.Display.print("_");

    M5.Display.fillRect(0, 122, 240, 13, TFT_NAVY);
    M5.Display.setTextColor(TFT_WHITE, TFT_NAVY);
    M5.Display.setCursor(4, 124);
    M5.Display.print(T("Aa大写 回车连接 Fn+`取消", "Aa:Upper ENTER:Connect Fn+`:Cancel"));
}

void drawSettingsPage() {
    M5.Display.fillScreen(TFT_BLACK);
    drawTopBar();
    M5.Display.setFont(&fonts::efontCN_12);

    M5.Display.fillRect(0, 22, 240, 20, TFT_DARKGREY);
    M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREY);
    M5.Display.setCursor(6, 24);
    if (settingsSubMenu == 0) M5.Display.print(T("设置", "Settings"));
    else if (settingsSubMenu == 1) M5.Display.print(T("语言", "Language"));
    else M5.Display.print(T("帮助", "Help"));

    if (settingsSubMenu == 0) {
        const char* items[] = { "", "" };
        items[0] = T("语言 / Language", "Language / 语言");
        items[1] = T("帮助 / Help", "Help / 帮助");
        int y = 55;
        for (int i = 0; i < 2; i++) {
            bool sel = (i == settingsSelected);
            uint16_t bg = sel ? TFT_BLUE : TFT_BLACK;
            if (sel) M5.Display.fillRect(0, y - 2, 240, 22, bg);
            M5.Display.setTextColor(sel ? TFT_WHITE : TFT_LIGHTGREY, bg);
            M5.Display.setCursor(10, y);
            M5.Display.print(items[i]);
            y += 28;
        }
    }
    else if (settingsSubMenu == 1) {
        const char* items[] = { "  Chinese / 中文", "  English", T("  退出 / Exit", "  Exit / 退出") };
        int y = 48;
        for (int i = 0; i < 3; i++) {
            bool sel = (i == langSelected);
            uint16_t bg = sel ? TFT_BLUE : TFT_BLACK;
            if (sel) M5.Display.fillRect(0, y - 2, 240, 22, bg);
            M5.Display.setTextColor(sel ? TFT_WHITE : TFT_LIGHTGREY, bg);
            M5.Display.setCursor(10, y);
            M5.Display.print(items[i]);

            if ((i == 0 && currentLang == LANG_CN) || (i == 1 && currentLang == LANG_EN)) {
                M5.Display.setTextColor(TFT_GREEN, bg);
                M5.Display.setCursor(220, y);
                M5.Display.print("*");
            }
            y += 24;
        }
    }
    else if (settingsSubMenu == 2) {
        M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        int y = 46;
        M5.Display.setCursor(4, y);
        M5.Display.print(T("主界面:", "Main:"));
        y += 13;
        M5.Display.setCursor(12, y);
        M5.Display.print(T("↑↓选择  回车详情", "↑↓Select  ENTER:Detail"));
        y += 13;
        M5.Display.setCursor(12, y);
        M5.Display.print(T("空格重扫  H设置", "SPACE:Rescan  H:Settings"));
        y += 15;
        M5.Display.setCursor(4, y);
        M5.Display.print(T("详情/密码/设置:", "Detail/Input/Settings:"));
        y += 13;
        M5.Display.setCursor(12, y);
        M5.Display.print(T("↑↓切换  回车确认", "↑↓Switch  ENTER:OK"));
        y += 13;
        M5.Display.setCursor(12, y);
        M5.Display.print(T("Fn+` 返回/取消", "Fn+` Back/Cancel"));
    }

    M5.Display.fillRect(0, 122, 240, 13, TFT_NAVY);
    M5.Display.setTextColor(TFT_WHITE, TFT_NAVY);
    M5.Display.setCursor(4, 124);
    if (settingsSubMenu == 0) {
        M5.Display.print(T("↑↓选择 回车进入 Fn+`返回", "↑↓Select ENTER:Enter Fn+`:Back"));
    } else if (settingsSubMenu == 1) {
        M5.Display.print(T("↑↓选择 回车确认 Fn+`返回", "↑↓Select ENTER:OK Fn+`:Back"));
    } else {
        M5.Display.print(T("Fn+` 或 回车 退出", "Fn+` or ENTER: Exit"));
    }
}

void tryConnect() {
    if (selectedIndex >= apCount) return;
    APInfo& ap = apList[selectedIndex];
    statusMsg = T("连接中...", "Connecting...");
    isConnecting = true;
    drawInputPage();

    WiFi.begin(ap.ssid.c_str(), passwordInput.c_str());
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(500); retries++;
        M5.Display.drawRect(10, 50, 220, 30, (retries % 2) ? TFT_YELLOW : TFT_WHITE);
    }

    if (WiFi.status() == WL_CONNECTED) {
        statusMsg = T("同步时间...", "Syncing time...");
        drawInputPage();
        configTime(8 * 3600, 0, "ntp.aliyun.com", "pool.ntp.org");
        time_t now = time(nullptr);
        int t = 0;
        while (now < 24 * 3600 && t < 20) { delay(500); now = time(nullptr); t++; }
        timeSynced = true;

        if (ap.ssid.length() > 0 && ap.ssid.length() <= 15) {
            prefs.begin("wifi_list", false);
            prefs.putString(ap.ssid.c_str(), passwordInput);
            prefs.end();
        }
        statusMsg = T("已连接", "Connected");
        isConnecting = false;
        currentState = STATE_DETAIL;
        drawDetailPage();
    } else {
        statusMsg = T("连接失败", "Failed");
        isConnecting = false;
        currentState = STATE_INPUT;
        drawInputPage();
    }
}

void saveWiFiInfo() {
    if (selectedIndex >= apCount) return;
    APInfo& ap = apList[selectedIndex];
    prefs.begin("wifi_list", false);
    if (ap.ssid.length() > 0 && ap.ssid.length() <= 15) {
        prefs.putString(ap.ssid.c_str(), passwordInput);
        statusMsg = T("保存成功!", "Saved!");
    } else {
        statusMsg = T("SSID 过长，无法保存", "SSID too long, cannot save");
    }
    prefs.end();
    drawDetailPage();
    delay(1200);
    statusMsg = "";
    drawDetailPage();
}

bool isEscKey(Keyboard_Class::KeysState& status) {
    if (status.fn) {
        for (auto c : status.word) {
            if (c == '`' || c == '~' || c == 27) return true;
        }
    }
    for (auto c : status.word) {
        if (c == 27) return true;
    }
    return false;
}

void handleKey(Keyboard_Class::KeysState& status) {
    bool escPressed = isEscKey(status);

    if (currentState == STATE_SETTINGS) {
        if (settingsSubMenu == 1) {
            if (escPressed) { settingsSubMenu = 0; drawSettingsPage(); return; }
            if (status.enter) {
                if (langSelected == 0) {
                    currentLang = LANG_CN;
                    saveLanguage(LANG_CN);
                    settingsSubMenu = 0;
                    drawSettingsPage();
                } else if (langSelected == 1) {
                    currentLang = LANG_EN;
                    saveLanguage(LANG_EN);
                    settingsSubMenu = 0;
                    drawSettingsPage();
                } else {
                    settingsSubMenu = 0;
                    drawSettingsPage();
                }
                return;
            }
            for (auto k : status.word) {
                if (k == ';') { if (langSelected > 0) langSelected--; drawSettingsPage(); }
                else if (k == '.') { if (langSelected < 2) langSelected++; drawSettingsPage(); }
            }
            return;
        }
        if (settingsSubMenu == 2) {
            if (escPressed || status.enter) { settingsSubMenu = 0; drawSettingsPage(); return; }
            return;
        }
        if (escPressed) { currentState = STATE_LIST; drawListPage(); return; }
        if (status.enter) {
            if (settingsSelected == 0) { settingsSubMenu = 1; langSelected = 0; drawSettingsPage(); }
            else if (settingsSelected == 1) { settingsSubMenu = 2; drawSettingsPage(); }
            return;
        }
        for (auto k : status.word) {
            if (k == ';') { if (settingsSelected > 0) settingsSelected--; drawSettingsPage(); }
            else if (k == '.') { if (settingsSelected < 1) settingsSelected++; drawSettingsPage(); }
        }
        return;
    }

    if (currentState == STATE_LIST) {
        if (status.space) { doScan(); drawListPage(); return; }
        if (status.enter && apCount > 0) {
            currentState = STATE_DETAIL; selectedButton = 0; statusMsg = "";
            drawDetailPage(); return;
        }
        for (auto k : status.word) {
            if (k == 'h' || k == 'H') {
                currentState = STATE_SETTINGS;
                settingsSubMenu = 0;
                settingsSelected = 0;
                drawSettingsPage();
                return;
            }
            if (k == ';') { if (selectedIndex > 0) { selectedIndex--; drawListPage(); } }
            else if (k == '.') { if (selectedIndex < apCount - 1) { selectedIndex++; drawListPage(); } }
        }
    }
    else if (currentState == STATE_DETAIL) {
        if (escPressed) { currentState = STATE_LIST; statusMsg = ""; drawListPage(); return; }
        if (status.enter) {
            if (selectedButton == 0) { currentState = STATE_INPUT; passwordInput = ""; statusMsg = ""; drawInputPage(); }
            else if (selectedButton == 1) { saveWiFiInfo(); }
            else if (selectedButton == 2) { currentState = STATE_LIST; statusMsg = ""; drawListPage(); }
            return;
        }
        for (auto k : status.word) {
            if (k == ';') { if (selectedButton > 0) selectedButton--; drawDetailPage(); }
            else if (k == '.') { if (selectedButton < 2) selectedButton++; drawDetailPage(); }
        }
    }
    else if (currentState == STATE_INPUT) {
        if (escPressed) { currentState = STATE_DETAIL; statusMsg = T("已取消", "Cancelled"); drawDetailPage(); return; }
        if (status.del && passwordInput.length() > 0) {
            passwordInput.remove(passwordInput.length() - 1); drawInputPage(); return;
        }
        if (status.enter) { if (!isConnecting) tryConnect(); return; }
        if (status.space) { passwordInput += " "; drawInputPage(); return; }
        bool added = false;
        for (auto c : status.word) {
            if (c >= 32 && c <= 126 && c != '`') { passwordInput += c; added = true; }
        }
        if (added) drawInputPage();
    }
}

void setup() {
    Serial.begin(115200);
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5.Display.setRotation(1);
    M5.Display.setBrightness(80);
    M5.Display.setFont(&fonts::efontCN_12);

    bootFeedback();

    loadLanguage();

    drawBootScreen();

    doScan();
    drawListPage();
}

void loop() {
    M5Cardputer.update();

    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
        handleKey(status);
    }

    if (WiFi.status() == WL_CONNECTED && timeSynced && millis() - lastTimeUpdate > 1000) {
        lastTimeUpdate = millis();
        if (currentState == STATE_LIST || currentState == STATE_DETAIL) drawTopBar();
    }

    if (millis() - lastBatteryCheck > 5000) {
        lastBatteryCheck = millis();
        int level = M5.Power.getBatteryLevel();
        if (level != lastBatteryLevel) {
            lastBatteryLevel = level;
            if (currentState == STATE_LIST || currentState == STATE_DETAIL) drawTopBar();
        }
    }

    static unsigned long lastBlink = 0;
    if (currentState == STATE_INPUT && !isConnecting && millis() - lastBlink > 400) {
        lastBlink = millis();
        drawInputPage();
    }

    delay(5);
}