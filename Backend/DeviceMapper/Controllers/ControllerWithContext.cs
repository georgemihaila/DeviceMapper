using Microsoft.AspNetCore.Mvc;

using DeviceMapper.Infra.Database;

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

namespace DeviceMapper.Controllers
{
    public abstract class ControllerWithContext : ControllerBase
    {
        protected ControllerWithContext(DeviceMapperContext context)
        {
            Context = context;
        }

        protected DeviceMapperContext Context { get; private set; }
    }
}
