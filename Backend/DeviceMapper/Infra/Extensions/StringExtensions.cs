using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

namespace DeviceMapper.Backend.Infra.Extensions
{
    public static class StringExtensions
    {
        public static string AtOrEmpty(this string[] groups, int index)
        {
            if (groups.Length > index - 1)
            {
                return groups[index];
            }
            return string.Empty;
        }
    }
}
