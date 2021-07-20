#ifndef HTML_TEMPLATE_H
#define HTML_TEMPLATE_H

#define HTML_TEMPLATE_NETCONFIG "<html>" \
    "<head><title>ESP32 Network Configuration</title></head><body>" \
    "<h1>ESP32 Network Configuration</h1><br>" \
    "<form action=\"/netconfig\" method=\"post\">" \
    "<label for=\"ssid\">SSID</label><br>" \
    "<input type=\"text\" id=\"ssid\" name=\"ssid\"><br>" \
    "<label for=\"psk\">Password (WPA2 PSK)</label><br>" \
    "<input type=\"password\" id=\"psk\" name=\"psk\"><br>" \
    "<input type=\"submit\" value=\"Submit\">" \
    "</form></body></html>"

#endif