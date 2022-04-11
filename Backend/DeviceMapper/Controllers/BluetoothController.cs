
using Microsoft.AspNetCore.Mvc;

using DeviceMapper.Infra;
using DeviceMapper.Infra.Database;

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

namespace DeviceMapper.Controllers
{
    [ApiController]
    [Route("[controller]")]
    public class BluetoothController : ControllerWithContext
    {
        private readonly TelegramBot _telegramBot;
        public BluetoothController(DeviceMapperContext context, TelegramBot telegramBot) : base(context)
        {
            _telegramBot = telegramBot;
        }

        [HttpPost]
        [Route("[action]")]
        public async Task<IActionResult> ProcessRawStringAsync([FromBody] string[] data)
        {
            var message = new List<string>();
            foreach(var entry in data)
            {
                var groups = entry.Split(',');
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
                    message.Add($"{device.Name}[{device.Mac}] discovered");
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
                        message.Add($"{device.Name}[{device.Mac}] updated");
                    }
                }
                Context.SaveChanges();
                var deviceID = Context.BluetoothDevices.First(x => x.Mac == device.Mac).Id;
                Context.BluetoothSpots.Add(new BluetoothSpot()
                {
                    Date = DateTime.Now,
                    Device = deviceID
                });
                Context.SaveChanges();
            }
            message.Add($"Total: {Context.BluetoothDevices.Count()} Bluetooth devices");
            await _telegramBot.SendMessageIfSoonEnoughAsync(string.Join('\n', message));
            return StatusCode(201);
        }
    }
}
