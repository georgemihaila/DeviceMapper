using System;
using System.Collections.Generic;

#nullable disable

namespace DeviceMapper.Infra.Database
{
    public partial class BluetoothSpot
    {
        public int Id { get; set; }
        public int Device { get; set; }
        public int? Location { get; set; }
        public DateTime Date { get; set; }

        public virtual BluetoothDevice DeviceNavigation { get; set; }
        public virtual LocationDatum LocationNavigation { get; set; }
    }
}
