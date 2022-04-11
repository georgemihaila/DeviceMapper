using System;
using System.Collections.Generic;

#nullable disable

namespace DeviceMapper.Infra.Database
{
    public partial class LocationDatum
    {
        public LocationDatum()
        {
            BluetoothSpots = new HashSet<BluetoothSpot>();
            WiFiSpots = new HashSet<WiFiSpot>();
        }

        public int Id { get; set; }
        public decimal Latitude { get; set; }
        public decimal Longitude { get; set; }

        public virtual ICollection<BluetoothSpot> BluetoothSpots { get; set; }
        public virtual ICollection<WiFiSpot> WiFiSpots { get; set; }
    }
}
