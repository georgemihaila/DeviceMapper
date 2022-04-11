
using Microsoft.AspNetCore.Mvc;

using DeviceMapper.Infra.Database;

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

namespace DeviceMapper.Controllers
{
    [ApiController]
    [Route("[controller]")]
    public class WiFiController : ControllerWithContext
    {
        public WiFiController(DeviceMapperContext context) : base(context)
        {
        }

        [HttpPost]
        [Route("[action]")]
        public IActionResult ProcessRawString(string data)
        {
            return StatusCode(201);
        }
    }
}
