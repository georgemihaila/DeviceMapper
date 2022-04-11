using System;
using System.Collections.Generic;

#nullable disable

namespace DeviceMapper.Infra.Database
{
    public partial class BluetoothDevice
    {
        public BluetoothDevice()
        {
            BluetoothSpots = new HashSet<BluetoothSpot>();
        }

        public int Id { get; set; }
        public string Name { get; set; }
        public string Mac { get; set; }
        public string Type { get; set; }
        public string ManufacturerData { get; set; }
        public string ServiceUuid { get; set; }

        public virtual ICollection<BluetoothSpot> BluetoothSpots { get; set; }
    }
}
