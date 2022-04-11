using System;
using Microsoft.EntityFrameworkCore;
using Microsoft.EntityFrameworkCore.Metadata;

#nullable disable

namespace DeviceMapper.Infra.Database
{
    public partial class DeviceMapperContext : DbContext
    {
        public DeviceMapperContext()
        {
        }

        public DeviceMapperContext(DbContextOptions<DeviceMapperContext> options)
            : base(options)
        {
        }

        public virtual DbSet<BluetoothDevice> BluetoothDevices { get; set; }
        public virtual DbSet<BluetoothSpot> BluetoothSpots { get; set; }
        public virtual DbSet<LocationDatum> LocationData { get; set; }
        public virtual DbSet<WiFiNetwork> WiFiNetworks { get; set; }
        public virtual DbSet<WiFiSpot> WiFiSpots { get; set; }

        protected override void OnModelCreating(ModelBuilder modelBuilder)
        {
            modelBuilder.HasAnnotation("Relational:Collation", "SQL_Latin1_General_CP1_CI_AS");

            modelBuilder.Entity<BluetoothDevice>(entity =>
            {
                entity.HasIndex(e => e.Mac, "UQ__Bluetoot__C790778CE5BBE39B")
                    .IsUnique();

                entity.Property(e => e.Id).HasColumnName("ID");

                entity.Property(e => e.Mac)
                    .HasMaxLength(17)
                    .IsUnicode(false)
                    .HasColumnName("MAC");

                entity.Property(e => e.ManufacturerData).HasMaxLength(255);

                entity.Property(e => e.Name).HasMaxLength(255);

                entity.Property(e => e.ServiceUuid)
                    .HasMaxLength(255)
                    .HasColumnName("ServiceUUID");

                entity.Property(e => e.Type).HasMaxLength(255);
            });

            modelBuilder.Entity<BluetoothSpot>(entity =>
            {
                entity.Property(e => e.Id).HasColumnName("ID");

                entity.Property(e => e.Date).HasColumnType("datetime");

                entity.HasOne(d => d.DeviceNavigation)
                    .WithMany(p => p.BluetoothSpots)
                    .HasForeignKey(d => d.Device)
                    .OnDelete(DeleteBehavior.ClientSetNull)
                    .HasConstraintName("FK__Bluetooth__Devic__300424B4");

                entity.HasOne(d => d.LocationNavigation)
                    .WithMany(p => p.BluetoothSpots)
                    .HasForeignKey(d => d.Location)
                    .HasConstraintName("FK__Bluetooth__Locat__30F848ED");
            });

            modelBuilder.Entity<LocationDatum>(entity =>
            {
                entity.Property(e => e.Id).HasColumnName("ID");

                entity.Property(e => e.Latitude).HasColumnType("decimal(8, 6)");

                entity.Property(e => e.Longitude).HasColumnType("decimal(9, 6)");
            });

            modelBuilder.Entity<WiFiNetwork>(entity =>
            {
                entity.HasIndex(e => e.Bssid, "UQ__WiFiNetw__B6C0E36B5C049014")
                    .IsUnique();

                entity.Property(e => e.Id).HasColumnName("ID");

                entity.Property(e => e.Bssid)
                    .HasMaxLength(17)
                    .IsUnicode(false)
                    .HasColumnName("BSSID");

                entity.Property(e => e.Capabilities).IsUnicode(false);

                entity.Property(e => e.Security).HasMaxLength(255);

                entity.Property(e => e.Ssid)
                    .HasMaxLength(32)
                    .IsUnicode(false)
                    .HasColumnName("SSID");

                entity.Property(e => e.Type).HasMaxLength(255);
            });

            modelBuilder.Entity<WiFiSpot>(entity =>
            {
                entity.Property(e => e.Id).HasColumnName("ID");

                entity.Property(e => e.Date).HasColumnType("datetime");

                entity.HasOne(d => d.LocationNavigation)
                    .WithMany(p => p.WiFiSpots)
                    .HasForeignKey(d => d.Location)
                    .HasConstraintName("FK__WiFiSpots__Locat__2F10007B");

                entity.HasOne(d => d.NetworkNavigation)
                    .WithMany(p => p.WiFiSpots)
                    .HasForeignKey(d => d.Network)
                    .OnDelete(DeleteBehavior.ClientSetNull)
                    .HasConstraintName("FK__WiFiSpots__Netwo__2E1BDC42");
            });

            OnModelCreatingPartial(modelBuilder);
        }

        partial void OnModelCreatingPartial(ModelBuilder modelBuilder);
    }
}
