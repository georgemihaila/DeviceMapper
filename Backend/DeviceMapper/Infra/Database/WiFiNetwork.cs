using System;
using System.Collections.Generic;

#nullable disable

namespace DeviceMapper.Infra.Database
{
    public partial class WiFiNetwork
    {
        public WiFiNetwork()
        {
            WiFiSpots = new HashSet<WiFiSpot>();
        }

        public int Id { get; set; }
        public string Ssid { get; set; }
        public string Security { get; set; }
        public int? Channel { get; set; }
        public string Bssid { get; set; }
        public string Type { get; set; }
        public string Capabilities { get; set; }

        public virtual ICollection<WiFiSpot> WiFiSpots { get; set; }
    }
}
