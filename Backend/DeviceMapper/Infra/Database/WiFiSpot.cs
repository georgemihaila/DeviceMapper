using System;
using System.Collections.Generic;

#nullable disable

namespace DeviceMapper.Infra.Database
{
    public partial class WiFiSpot
    {
        public int Id { get; set; }
        public int Network { get; set; }
        public int? Location { get; set; }
        public DateTime Date { get; set; }

        public virtual LocationDatum LocationNavigation { get; set; }
        public virtual WiFiNetwork NetworkNavigation { get; set; }
    }
}
