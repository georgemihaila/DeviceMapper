
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
    public class LocationController : ControllerBase
    {
        [HttpPost]
        [Route("[action]")]
        public IActionResult UpdateLocation()
        {
            return StatusCode(201);
        }
    }
}
