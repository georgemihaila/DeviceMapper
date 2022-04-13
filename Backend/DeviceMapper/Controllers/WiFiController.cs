
using Microsoft.AspNetCore.Mvc;

using DeviceMapper.Infra.Database;

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using DeviceMapper.Infra;
using DeviceMapper.Backend.Infra.Extensions;
using DeviceMapper.Backend.Infra;

namespace DeviceMapper.Controllers
{
    [ApiController]
    [Route("[controller]")]
    public class WiFiController : CustomController
    {
        private static CircularList<string> _latestEntries = new CircularList<string>(20);

        public WiFiController(DeviceMapperContext context, TelegramBot telegramBot) : base(context, telegramBot, 1)
        {

        }

        /*
         * String data = String(WiFi.SSID(i));
    data += "," + String(WiFi.encryptionType(i));
    data += "," + String(WiFi.channel(i));
    data += "," + String(bssidToString(WiFi.BSSID(i)));
         */

        [HttpPost]
        [Route("[action]")]
        public async Task<IActionResult> ProcessRawString([FromBody] string data)
        {
            var bottom = $"\n\nTotal: {Context.WiFiNetworks.Count()} WiFi networks ({DateTime.Now:HH:mm:sstt})\n";
            var groups = data.Split(',');
            var location = GetOrCreateLocation(groups.AtOrEmpty(4), groups.AtOrEmpty(5), groups.AtOrEmpty(6));
            var locationString = string.Empty;
            if (location.Id != 1)
            {
                locationString = $" [Lo: {location.Longitude}, La: {location.Latitude}, Al: {location.Altitude}]";
            }
            var wifiNetwork = new WiFiNetwork()
            {
                Ssid = groups.AtOrEmpty(0),
                Security = groups.AtOrEmpty(1),
                Channel = int.Parse(groups.AtOrEmpty(2)),
                Bssid = groups.AtOrEmpty(3)
            };
            if (!Context.WiFiNetworks.Any(x => x.Ssid == wifiNetwork.Ssid))
            {
                Context.WiFiNetworks.Add(wifiNetwork);
                _latestEntries.Add($"{wifiNetwork.Ssid}[{wifiNetwork.Bssid}] discovered{locationString}");
                await CreateOrEditLastMessageAsync(string.Join("\n", _latestEntries) + bottom);
            }
            else
            {
                var target = Context.WiFiNetworks.First(x => x.Ssid == wifiNetwork.Ssid);
                var updated = false;
                if (target.Ssid != wifiNetwork.Ssid && !string.IsNullOrWhiteSpace(wifiNetwork.Ssid))
                {
                    target.Ssid = wifiNetwork.Ssid;
                    updated = true;
                }
                if (target.Security != wifiNetwork.Security)
                {
                    target.Security = wifiNetwork.Security;
                    updated = true;
                }
                if (target.Channel != wifiNetwork.Channel)
                {
                    target.Channel = wifiNetwork.Channel;
                    updated = true;
                }
                if (target.Bssid != wifiNetwork.Bssid)
                {
                    target.Bssid = wifiNetwork.Bssid;
                    updated = true;
                }
                if (updated)
                {
                    Context.Update(target);
                    _latestEntries.Add($"{wifiNetwork.Ssid}[{wifiNetwork.Bssid}] updated{locationString}");
                    await CreateOrEditLastMessageAsync(string.Join("\n", _latestEntries) + bottom);
                }
            }
            Context.SaveChanges();
            var deviceID = Context.WiFiNetworks.First(x => x.Ssid == wifiNetwork.Ssid).Id;
            Context.WiFiSpots.Add(new WiFiSpot()
            {
                Date = DateTime.Now,
                Network = deviceID,
                Location = location.Id
            });
            Context.SaveChanges();
            return StatusCode(201);
        }
    }
}
