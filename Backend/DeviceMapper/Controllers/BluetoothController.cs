
using Microsoft.AspNetCore.Mvc;

using DeviceMapper.Infra;
using DeviceMapper.Infra.Database;

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using DeviceMapper.Backend.Infra;
using DeviceMapper.Backend.Infra.Extensions;

namespace DeviceMapper.Controllers
{
    [ApiController]
    [Route("[controller]")]
    public class BluetoothController : CustomController
    {
        private static CircularList<string> _latestEntries = new CircularList<string>(20);
        private static int _sessionNew = 0;
        private static int _sessionUpdated = 0;

        public BluetoothController(DeviceMapperContext context, TelegramBot telegramBot) : base(context, telegramBot, 2)
        {

        }

        [HttpPost]
        [Route("[action]")]
        public async Task<IActionResult> ProcessRawStringAsync([FromBody] string data)
        {
            var bottom = $"\n\nTotal: {Context.BluetoothDevices.Count()} Bluetooth devices ({DateTime.Now:HH:mm:sstt})\n";
            var groups = data.Split(',');
            var location = GetOrCreateLocation(groups.AtOrEmpty(0), groups.AtOrEmpty(1), groups.AtOrEmpty(2));
            var locationString = groups.AtOrEmpty(0) + groups.AtOrEmpty(1) + groups.AtOrEmpty(2);
            var shouldSendMessage = false;
            if (location.Id != 1)
            {
                locationString = $" [Lo: {location.Longitude}, La: {location.Latitude}, Al: {location.Altitude}]";
            }
            var device = new BluetoothDevice()
            {
                Mac = groups.First(x => x.Contains(" Address: ")).Replace(" Address: ", string.Empty)
            };
            if (groups.Any(x => x.Contains(" serviceUUID: ")))
            {
                device.ServiceUuid = groups.First(x => x.Contains(" serviceUUID: ")).Replace(" serviceUUID: ", string.Empty);
            }
            if (groups.Any(x => x.Contains("Name")))
            {
                device.Name = groups.First(x => x.Contains("Name")).Replace("Name: ", string.Empty);
            }
            if (groups.Any(x => x.Contains(" manufacturer data: ")))
            {
                device.ManufacturerData = groups.First(x => x.Contains(" manufacturer data: ")).Replace("Name: ", string.Empty);
            }
            if (!Context.BluetoothDevices.Any(x => x.Mac == device.Mac))
            {
                Context.BluetoothDevices.Add(device);
                _latestEntries.Add($"{device.Name}[{device.Mac}] discovered{locationString}");
                _sessionNew++;
                shouldSendMessage = true;
            }
            else
            {
                var target = Context.BluetoothDevices.First(x => x.Mac == device.Mac);
                var updated = false;
                if (target.ManufacturerData != device.ManufacturerData)
                {
                    target.ManufacturerData = device.ManufacturerData;
                    updated = true;
                }
                if (target.Name != device.Name)
                {
                    target.Name = device.Name;
                    updated = true;
                }
                if (target.ServiceUuid != device.ServiceUuid)
                {
                    target.ServiceUuid = device.ServiceUuid;
                    updated = true;
                }
                if (target.Type != device.Type)
                {
                    target.Type = device.Type;
                    updated = true;
                }
                if (updated)
                {
                    Context.Update(target);
                    _latestEntries.Add($"{device.Name}[{device.Mac}] updated{locationString}");
                    _sessionUpdated++;
                    shouldSendMessage = true;
                }
            }
            Context.SaveChanges();
            var deviceID = Context.BluetoothDevices.First(x => x.Mac == device.Mac).Id;
            Context.BluetoothSpots.Add(new BluetoothSpot()
            {
                Date = DateTime.Now,
                Device = deviceID,
                Location = location.Id
            });
            Context.SaveChanges();
            if (shouldSendMessage)
            {
                await CreateOrEditLastMessageAsync(string.Join("\n", _latestEntries) + bottom + $"Session: {_sessionNew} new, {_sessionUpdated} updated\n");
            }
            return StatusCode(201);
        }
    }
}
