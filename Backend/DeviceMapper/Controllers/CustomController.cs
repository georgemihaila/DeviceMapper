using Microsoft.AspNetCore.Mvc;

using DeviceMapper.Infra.Database;

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using DeviceMapper.Infra;
using DeviceMapper.Backend.Infra;
using NetTelegramBotApi.Types;

namespace DeviceMapper.Controllers
{
    public abstract class CustomController : ControllerBase
    {
        private readonly TelegramBot _telegramBot;
        private readonly int _pinnedMessageIndex;

        protected CustomController(DeviceMapperContext context, TelegramBot telegramBot, int pinnedMessageIndex)
        {
            Context = context;
            _telegramBot = telegramBot;
            _pinnedMessageIndex = pinnedMessageIndex;
        }

        protected DeviceMapperContext Context { get; private set; }

        protected async Task<Message> CreateOrEditLastMessageAsync(string message)
        {
            return await _telegramBot.CreateOrEditLastMessageAsync(message, _pinnedMessageIndex);
        }

        protected LocationDatum GetOrCreateLocation(string longitudeString, string latitudeString, string altitudeString)
        {
            if (decimal.TryParse(longitudeString, out decimal longitude)) { }
            if (decimal.TryParse(latitudeString, out decimal latitude)) { }
            if (decimal.TryParse(altitudeString, out decimal altitude)) { }
            var location = new LocationDatum()
            {
                Longitude = longitude,
                Latitude = latitude,
                Altitude = altitude
            };
            if (!Context.LocationData.Any(x => x.Longitude == location.Longitude && x.Latitude == location.Latitude && x.Altitude == location.Altitude))
            {
                Context.LocationData.Add(location);
                Context.SaveChanges();
            }
            return Context.LocationData.First(x => x.Longitude == location.Longitude && x.Latitude == location.Latitude && x.Altitude == location.Altitude);
        }
    }
}
